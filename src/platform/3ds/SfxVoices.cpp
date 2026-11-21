#include "SfxVoices.hpp"
#include "SfxMixer.hpp"
#include <3ds.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include <SDL2/SDL_timer.h>

namespace {
constexpr unsigned Channel=2,Frames=512,Slots=3,QueuedBuffers=3;
constexpr unsigned BufferBytes=Frames*2*sizeof(int16_t);
struct Sample {std::string path;std::vector<int16_t> pcm;unsigned channels;};
struct Slot {ndspWaveBuf wave{};int16_t* data=nullptr;};
std::vector<Sample> samples;
Slot slots[Slots];
void* output=nullptr;
unsigned nextSlot=0;
std::atomic_bool active=false,resetRequested=false;
std::atomic<u32> resetFrame=0,requests=0;
std::atomic_int volume=100;
LightEvent audioEvent;
LightLock effectLock;
Thread worker=nullptr;
std::atomic_bool quit=false;
void Pump();
void MixThread(void*) {
 while(!quit.load(std::memory_order_acquire)) {
  LightEvent_Wait(&audioEvent);
  if(quit.load(std::memory_order_acquire))break;
  Pump();
 }
}
}

bool CtrSfx::Initialize(int level) {
 active.store(false,std::memory_order_release);
 if(!output)output=linearAlloc(Slots*BufferBytes);
 if(!output)return false;
 requests=0;volume=level;nextSlot=0;
 LightEvent_Init(&audioEvent,RESET_ONESHOT);LightLock_Init(&effectLock);quit=false;
 CtrSfxMixer::ResetVoices();
 for(unsigned i=0;i<Slots;i++) {
  slots[i]={};slots[i].data=static_cast<int16_t*>(output)+i*Frames*2;
 }
 ndspChnReset(Channel);
 ndspChnSetFormat(Channel,NDSP_FORMAT_STEREO_PCM16);
 ndspChnSetRate(Channel,CtrSfxMixer::SampleRate);
 ndspChnSetInterp(Channel,NDSP_INTERP_POLYPHASE);
 float mix[12]{};mix[0]=mix[1]=1;ndspChnSetMix(Channel,mix);
 active.store(true,std::memory_order_release);
 worker=threadCreate(MixThread,nullptr,32*1024,0x19,-1,false);
 if(!worker){Shutdown();return false;}
 LightEvent_Signal(&audioEvent);
 return true;
}
void CtrSfx::StopRequests() {active.store(false,std::memory_order_release);requests=0;}
void CtrSfx::BeginReload() {
 if(!worker){Shutdown();return;}
 StopRequests();resetFrame=0;resetRequested.store(true,std::memory_order_release);
 LightEvent_Signal(&audioEvent);
 // The callback acknowledges the clear before sample/output memory is freed.
 while(!resetFrame.load(std::memory_order_acquire))SDL_Delay(1);
 const u32 frame=resetFrame.load(std::memory_order_relaxed)-1;
 while(u32(ndspGetFrameCount()-frame)<2)SDL_Delay(1);
 Shutdown();
}
void CtrSfx::Shutdown() {
 // Either ndspExit joined the callback, or BeginReload acknowledged its stop.
 StopWorker();
 CtrSfxMixer::Clear();samples.clear();resetRequested=false;
 if(output)linearFree(output);output=nullptr;
 for(auto& slot:slots)slot={};
}
void CtrSfx::StopWorker() {
 StopRequests();
 resetRequested.store(false,std::memory_order_release);
 quit.store(true,std::memory_order_release);
 if(worker){LightEvent_Signal(&audioEvent);threadJoin(worker,U64_MAX);threadFree(worker);worker=nullptr;}
}

bool CtrSfx::Load(int id,const char* filename,float gain) {
 if(id<0 || id>=int(CtrSfxMixer::EffectCount))return false;
 for(const auto& sample:samples)if(sample.path==filename) {
  if(worker)LightLock_Lock(&effectLock);
  CtrSfxMixer::SetEffect(id,sample.pcm.data(),sample.pcm.size()/sample.channels,sample.channels,gain);
  if(worker)LightLock_Unlock(&effectLock);
  return true;
 }
 FILE* file=fopen(filename,"rb");if(!file)return false;
 u32 header[4];bool ok=fread(header,1,sizeof(header),file)==sizeof(header) &&
  header[0]==0x334d4350 && header[1]>=8000 && header[1]<=48000 &&
  header[2]>=1 && header[2]<=2 && header[3]>0 && header[3]<48000*30;
 std::vector<int16_t> pcm;
 if(ok) {
  pcm.resize(size_t(header[3])*header[2]);
  const size_t bytes=pcm.size()*sizeof(int16_t);
  ok=fread(pcm.data(),1,bytes,file)==bytes;
 }
 fclose(file);if(!ok)return false;
 CtrSfxMixer::PreparePcm(pcm,header[1],header[2]);
 samples.push_back({filename,std::move(pcm),header[2]});
 const auto& sample=samples.back();
 if(worker)LightLock_Lock(&effectLock);
 CtrSfxMixer::SetEffect(id,sample.pcm.data(),sample.pcm.size()/sample.channels,sample.channels,gain);
 if(worker)LightLock_Unlock(&effectLock);
 return true;
}
void CtrSfx::Request(int id) {
 if(!active.load(std::memory_order_acquire) || id<0 || id>=int(CtrSfxMixer::EffectCount))return;
 // Coalesce triggers, never accumulate an audio event backlog.
 requests.fetch_or(1u<<id,std::memory_order_release);
}
void CtrSfx::SetVolume(int level) {volume.store(level,std::memory_order_relaxed);}
void CtrSfx::OnAudioFrame() {
 if(active.load(std::memory_order_acquire) || resetRequested.load(std::memory_order_acquire))
  LightEvent_Signal(&audioEvent);
}
namespace {
void Pump() {
 if(resetRequested.exchange(false,std::memory_order_acquire)) {
  ndspChnWaveBufClear(Channel);
  resetFrame.store(ndspGetFrameCount()+1,std::memory_order_release);return;
 }
 if(!active.load(std::memory_order_acquire))return;
 unsigned queued=0;
 for(const auto& slot:slots)
  if(slot.wave.status==NDSP_WBUF_QUEUED || slot.wave.status==NDSP_WBUF_PLAYING)++queued;
 if(queued>=QueuedBuffers)return;
 LightLock_Lock(&effectLock);
 CtrSfxMixer::Trigger(requests.exchange(0,std::memory_order_acquire));
 // Three 11.6 ms buffers cover worker scheduling and DSP handoff jitter.
 while(queued<QueuedBuffers) {
  auto& slot=slots[nextSlot];
  if(slot.wave.status==NDSP_WBUF_QUEUED || slot.wave.status==NDSP_WBUF_PLAYING)break;
  CtrSfxMixer::Render(slot.data,Frames,volume.load(std::memory_order_relaxed));
  DSP_FlushDataCache(slot.data,BufferBytes);
  slot.wave={};slot.wave.data_pcm16=slot.data;slot.wave.nsamples=Frames;
  ndspChnWaveBufAdd(Channel,&slot.wave);
  nextSlot=(nextSlot+1)%Slots;++queued;
 }
 LightLock_Unlock(&effectLock);
}
}

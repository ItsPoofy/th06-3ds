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
constexpr unsigned Channel=2,Frames=256,Slots=3,QueuedBuffers=2;
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
}

bool CtrSfx::Initialize(int level) {
 active.store(false,std::memory_order_release);
 if(!output)output=linearAlloc(Slots*BufferBytes);
 if(!output)return false;
 requests=0;volume=level;nextSlot=0;
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
 return true;
}
void CtrSfx::StopRequests() {active.store(false,std::memory_order_release);requests=0;}
void CtrSfx::BeginReload() {
 StopRequests();resetFrame=0;resetRequested.store(true,std::memory_order_release);
 // The callback acknowledges the clear before sample/output memory is freed.
 while(!resetFrame.load(std::memory_order_acquire)) {if(aptShouldClose())return;SDL_Delay(1);}
 const u32 frame=resetFrame.load(std::memory_order_relaxed)-1;
 while(u32(ndspGetFrameCount()-frame)<2) {if(aptShouldClose())return;SDL_Delay(1);}
 Shutdown();
}
void CtrSfx::Shutdown() {
 // Either ndspExit joined the callback, or BeginReload acknowledged its stop.
 StopRequests();CtrSfxMixer::Clear();samples.clear();
 if(output)linearFree(output);output=nullptr;
 for(auto& slot:slots)slot={};
}

bool CtrSfx::Load(int id,const char* filename,float gain) {
 if(id<0 || id>=int(CtrSfxMixer::EffectCount))return false;
 for(const auto& sample:samples)if(sample.path==filename) {
  CtrSfxMixer::SetEffect(id,sample.pcm.data(),sample.pcm.size()/sample.channels,sample.channels,gain);
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
 CtrSfxMixer::SetEffect(id,sample.pcm.data(),sample.pcm.size()/sample.channels,sample.channels,gain);
 return true;
}
void CtrSfx::Request(int id) {
 if(!active.load(std::memory_order_acquire) || id<0 || id>=int(CtrSfxMixer::EffectCount))return;
 // Coalesce triggers, never accumulate an audio event backlog.
 requests.fetch_or(1u<<id,std::memory_order_release);
}
void CtrSfx::SetVolume(int level) {volume.store(level,std::memory_order_relaxed);}
void CtrSfx::OnAudioFrame() {
 if(resetRequested.exchange(false,std::memory_order_acquire)) {
  ndspChnWaveBufClear(Channel);
  resetFrame.store(ndspGetFrameCount()+1,std::memory_order_release);return;
 }
 if(!active.load(std::memory_order_acquire))return;
 unsigned queued=0;
 for(const auto& slot:slots)
  if(slot.wave.status==NDSP_WBUF_QUEUED || slot.wave.status==NDSP_WBUF_PLAYING)++queued;
 if(queued>=QueuedBuffers)return;
 CtrSfxMixer::Trigger(requests.exchange(0,std::memory_order_acquire));
 // At most two 5.8 ms buffers are outstanding. A third slot permits reuse
 // only after DSP completion, without a long latency-hiding audio queue.
 while(queued<QueuedBuffers) {
  auto& slot=slots[nextSlot];
  if(slot.wave.status==NDSP_WBUF_QUEUED || slot.wave.status==NDSP_WBUF_PLAYING)break;
  CtrSfxMixer::Render(slot.data,Frames,volume.load(std::memory_order_relaxed));
  DSP_FlushDataCache(slot.data,BufferBytes);
  slot.wave={};slot.wave.data_pcm16=slot.data;slot.wave.nsamples=Frames;
  ndspChnWaveBufAdd(Channel,&slot.wave);
  nextSlot=(nextSlot+1)%Slots;++queued;
 }
}

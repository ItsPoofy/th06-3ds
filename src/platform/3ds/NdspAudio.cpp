#include "NdspAudio.hpp"
#include "SfxVoices.hpp"
#include "AssetPath.hpp"
#include "Settings.hpp"
#include "Runtime.hpp"
#include <3ds.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
constexpr unsigned BlockBytes=8192,Slots=3;
struct Block {u32 samples,bytes,offset;ndspAdpcmData context[2];};
static_assert(sizeof(Block)==24);
struct Slot {ndspWaveBuf wave[2]{};ndspAdpcmData context[2]{};u8* data[2]{};};
Slot slots[Slots];
FILE* music=nullptr;std::vector<Block> blocks;u16 coefficients[2][16]{};
u32 rate=0,nextBlock=0,loopBlock=0;unsigned nextSlot=0;
bool initialized=false,playing=false,looping=false;
float lastMusicGain=-1;
u64 clearedAt=0,fadeStart=0,fadeDuration=0;
SDL_Thread* worker=nullptr;std::atomic_bool quit=false;
LightLock musicLock;
LightEvent streamEvent;
std::atomic_bool streamActive=false,fadeActive=false;
std::atomic_int musicVolume=100;
void OnAudioFrame(void*) {
 CtrSfx::OnAudioFrame();
 // This runs on NDSP's high-priority thread. Never read files or take the
 // stream lock here; simply notify the worker after the DSP releases a slot.
 if(!streamActive.load(std::memory_order_acquire))return;
 if(fadeActive.load(std::memory_order_relaxed)) {LightEvent_Signal(&streamEvent);return;}
 for(const auto& slot:slots)
  if(slot.wave[0].status==NDSP_WBUF_DONE && slot.wave[1].status==NDSP_WBUF_DONE) {
   LightEvent_Signal(&streamEvent);return;
  }
}
struct Guard {Guard(){LightLock_Lock(&musicLock);}~Guard(){LightLock_Unlock(&musicLock);}};
std::string Path(const char* input,const char* ext) {
 auto name=CtrAssets::Normalize(input);name=name.substr(name.find_last_of('/')+1);
 if(auto dot=name.find_last_of('.');dot!=std::string::npos)name.resize(dot);
 return TH06_ASSET_ROOT "audio/"+name+ext;
}
void Mix(int channel,float left,float right) {float mix[12]{};mix[0]=left;mix[1]=right;ndspChnSetMix(channel,mix);}
void StopLocked() {
 streamActive.store(false,std::memory_order_release);fadeActive=false;
 playing=false;fadeStart=fadeDuration=0;lastMusicGain=-1;
 for(int c=0;c<2;c++){ndspChnSetPaused(c,true);ndspChnWaveBufClear(c);}
 clearedAt=ndspGetFrameCount();
 if(music)fclose(music);music=nullptr;blocks.clear();
}
bool Fill(unsigned slot) {
 if(nextBlock>=blocks.size()) {if(!looping)return false;nextBlock=loopBlock;}
 const auto& b=blocks[nextBlock++];auto& s=slots[slot];
 if(fseek(music,b.offset,SEEK_SET)!=0)return false;
 for(int c=0;c<2;c++) {
  if(fread(s.data[c],1,b.bytes,music)!=b.bytes)return false;
  DSP_FlushDataCache(s.data[c],b.bytes);
  s.context[c]=b.context[c];s.wave[c]={};s.wave[c].data_adpcm=s.data[c];s.wave[c].nsamples=b.samples;s.wave[c].adpcm_data=&s.context[c];
 }
 for(int c=0;c<2;c++)ndspChnWaveBufAdd(c,&s.wave[c]);
 return true;
}
int StreamThread(void*) {
 while(!quit) {
  LightEvent_Wait(&streamEvent);
  if(quit)break;
  {Guard guard;
   if(playing) {
    float gain=musicVolume.load(std::memory_order_relaxed)*.01f;
    if(fadeDuration) {
     const u64 elapsed=osGetTime()-fadeStart;
     if(elapsed>=fadeDuration){StopLocked();gain=0;}
     else gain*=std::pow(10.f,-2.5f*float(elapsed)/fadeDuration);
    }
    if(gain!=lastMusicGain){Mix(0,gain,0);Mix(1,0,gain);lastMusicGain=gain;}
    for(unsigned refilled=0;playing && refilled<Slots;refilled++) {
     auto& s=slots[nextSlot];
     if(s.wave[0].status!=NDSP_WBUF_DONE || s.wave[1].status!=NDSP_WBUF_DONE)break;
     if(Fill(nextSlot))nextSlot=(nextSlot+1)%Slots;
     else {
      if(nextBlock<blocks.size() || looping){CtrRuntime::Log("NDSP: music read failed");StopLocked();}
      else if(!ndspChnIsPlaying(0) && !ndspChnIsPlaying(1))StopLocked();
      break;
     }
    }
   }
  }
 }
 return 0;
}
}
bool CtrAudio::Initialize() {
 if(initialized)return true;
 if(R_FAILED(ndspInit())){CtrRuntime::Log("NDSP initialization failed");return false;}
 initialized=true;LightLock_Init(&musicLock);LightEvent_Init(&streamEvent,RESET_ONESHOT);
 musicVolume=CtrSettings::BgmVolume();streamActive=false;fadeActive=false;
 if(!CtrSfx::Initialize(CtrSettings::SfxVolume())){Shutdown();return false;}
 ndspSetOutputMode(NDSP_OUTPUT_STEREO);
 for(auto& s:slots)for(int c=0;c<2;c++) {
  s.data[c]=static_cast<u8*>(linearAlloc(BlockBytes));
  if(!s.data[c]){Shutdown();return false;}
 }
 quit=false;worker=SDL_CreateThread(StreamThread,"th06-ndsp",nullptr);
 if(!worker){Shutdown();return false;}
 ndspSetCallback(OnAudioFrame,nullptr);
 CtrRuntime::Log("NDSP: DSP-ADPCM music and 32-voice software-mixed PCM16 effects");return true;
}
void CtrAudio::Shutdown() {
 if(!initialized)return;
 CtrSfx::StopRequests();
 streamActive=false;quit=true;LightEvent_Signal(&streamEvent);
 if(worker)SDL_WaitThread(worker,nullptr);worker=nullptr;
 {Guard guard;StopLocked();}
 for(int ch=0;ch<24;ch++)ndspChnWaveBufClear(ch);
 // Keep the static callback valid until NDSP's own thread has stopped.
 ndspExit();ndspSetCallback(nullptr,nullptr);initialized=false;
 for(auto& s:slots)for(auto& data:s.data){if(data)linearFree(data);data=nullptr;}
 CtrSfx::Shutdown();
}
bool CtrAudio::LoadMusic(const char* path) {
 if(!initialized)return false;Guard guard;StopLocked();
 music=fopen(Path(path,".ndsp").c_str(),"rb");if(!music)return false;
 u32 header[7];bool ok=fread(header,1,sizeof(header),music)==sizeof(header) && header[0]==0x5053444e && header[1]==1 && header[3]==2 && header[2]>=8000 && header[2]<=48000 && header[4]>0 && header[4]<10000 && header[5]<header[4];
 if(ok) {
  rate=header[2];loopBlock=header[5];blocks.resize(header[4]);
  ok=fread(coefficients,1,sizeof(coefficients),music)==sizeof(coefficients) && fread(blocks.data(),sizeof(Block),blocks.size(),music)==blocks.size();
  fseek(music,0,SEEK_END);const long size=ftell(music);
  for(const auto& b:blocks)ok=ok && b.samples>0 && b.samples<=14336 && b.bytes==((b.samples+13)/14)*8 && b.offset>=92+blocks.size()*24 && u64(b.offset)+2*b.bytes<=u64(size) && (b.context[0].index>>4)<8 && (b.context[1].index>>4)<8;
 }
 if(!ok){CtrRuntime::Log("NDSP: invalid music asset");StopLocked();return false;}
 return true;
}
bool CtrAudio::HasLoop(){if(!initialized)return false;Guard guard;return music && loopBlock<blocks.size();}
bool CtrAudio::PlayMusic(bool loop) {
 if(!initialized || aptShouldClose())return false;Guard guard;if(!music || blocks.empty())return false;
 // The DSP must acknowledge queue clearing before those fixed buffers are reused.
 while(u32(ndspGetFrameCount()-clearedAt)<2) {
  if(quit.load(std::memory_order_acquire) || aptShouldClose())return false;
  SDL_Delay(1);
 }
 looping=loop;nextBlock=0;nextSlot=0;fadeStart=fadeDuration=0;fadeActive=false;
 musicVolume=CtrSettings::BgmVolume();
 for(int c=0;c<2;c++) {
  ndspChnReset(c);ndspChnSetPaused(c,true);ndspChnSetFormat(c,NDSP_FORMAT_MONO_ADPCM);
  ndspChnSetRate(c,rate);ndspChnSetInterp(c,NDSP_INTERP_POLYPHASE);ndspChnSetAdpcmCoefs(c,coefficients[c]);
  const float gain=CtrSettings::BgmVolume()*.01f;Mix(c,c==0?gain:0,c==1?gain:0);
 }
 for(unsigned i=0;i<Slots;i++)if(!Fill(i)){StopLocked();return false;}
 // Queue both stereo channels while paused, then release them in one DSP update.
 ndspChnSetPaused(0,false);ndspChnSetPaused(1,false);playing=true;
 streamActive.store(true,std::memory_order_release);LightEvent_Signal(&streamEvent);return true;
}
void CtrAudio::StopMusic(){if(initialized){Guard guard;StopLocked();}}
void CtrAudio::FadeMusic(float seconds) {
 if(!initialized)return;Guard guard;
 if(seconds<=0)StopLocked();
 else {fadeStart=osGetTime();fadeDuration=u64(seconds*1000);fadeActive=true;LightEvent_Signal(&streamEvent);}
}
bool CtrAudio::LoadEffect(int id,const char* path,float gain) {
 auto filename=Path(path,".pcm");
 const auto slash=filename.find_last_of('/');
 filename=TH06_ASSET_ROOT "variants/sound-"+std::string(CtrSettings::SoundPackName())+"/audio/"+filename.substr(slash+1);
 if(!initialized)return false;
 if(CtrSfx::Load(id,filename.c_str(),gain))return true;
 if(CtrSettings::SoundPack()!=0 && CtrSfx::Load(id,Path(path,".pcm").c_str(),gain))return true;
 char error[256];std::snprintf(error,sizeof(error),"NDSP: failed to load SFX %d: %s; linear free %lu KiB",id,filename.c_str(),static_cast<unsigned long>(linearSpaceFree()/1024));
 CtrRuntime::Log(error);return false;
}
void CtrAudio::PlayEffect(int id) {if(initialized)CtrSfx::Request(id);}
void CtrAudio::UpdateEffects() {
 if(!initialized)return;
 const int bgm=CtrSettings::BgmVolume();
 if(musicVolume.exchange(bgm,std::memory_order_relaxed)!=bgm && streamActive)
  LightEvent_Signal(&streamEvent);
 CtrSfx::SetVolume(CtrSettings::SfxVolume());
}

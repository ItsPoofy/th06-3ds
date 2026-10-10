#include "SoundPlayer.hpp"
#include "NdspAudio.hpp"
#include "GameErrorContext.hpp"
#include "i18n.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
static const SoundBufferIdxVolume g_SoundBufferIdxVol[32] = {
    {0, -1500}, {0, -2000}, {1, -1200}, {1, -1400}, {2, -1000},  {3, -500},   {4, -500},   {5, -1700},
    {6, -1700}, {7, -1700}, {8, -1000}, {9, -1000}, {10, -1900}, {11, -1200}, {12, -900},  {5, -1500},
    {13, -900}, {14, -900}, {15, -600}, {16, -400}, {17, -1100}, {18, -900},  {5, -1800},  {6, -1800},
    {7, -1800}, {19, -300}, {20, -600}, {21, -800}, {22, -100},  {23, -500},  {24, -1000}, {25, -1000},
};
static const char *const g_SFXList[26] = {
    "data/wav/plst00.wav", "data/wav/enep00.wav",   "data/wav/pldead00.wav", "data/wav/power0.wav",
    "data/wav/power1.wav", "data/wav/tan00.wav",    "data/wav/tan01.wav",    "data/wav/tan02.wav",
    "data/wav/ok00.wav",   "data/wav/cancel00.wav", "data/wav/select00.wav", "data/wav/gun00.wav",
    "data/wav/cat00.wav",  "data/wav/lazer00.wav",  "data/wav/lazer01.wav",  "data/wav/enep01.wav",
    "data/wav/nep00.wav",  "data/wav/damage00.wav", "data/wav/item00.wav",   "data/wav/kira00.wav",
    "data/wav/kira01.wav", "data/wav/kira02.wav",   "data/wav/extend.wav",   "data/wav/timeout.wav",
    "data/wav/graze.wav",  "data/wav/powerup.wav",
};

SoundPlayer g_SoundPlayer;
SoundPlayer::SoundPlayer() {std::fill(std::begin(soundBuffersToPlay),std::end(soundBuffersToPlay),-1);}
ZunResult SoundPlayer::InitializeDSound() {audioDev=CtrAudio::Initialize()?1:0;return audioDev?ZUN_SUCCESS:ZUN_ERROR;}
ZunResult SoundPlayer::Release() {CtrAudio::Shutdown();audioDev=0;return ZUN_SUCCESS;}
ZunResult SoundPlayer::LoadSound(i32 id,const char* path,f32 gain) {return CtrAudio::LoadEffect(id,path,gain)?ZUN_SUCCESS:ZUN_ERROR;}
ZunResult SoundPlayer::InitSoundBuffers() {
 std::fill(std::begin(soundBuffersToPlay),std::end(soundBuffersToPlay),-1);
 for(int i=0;i<32;i++) {
  const char* path=g_SFXList[g_SoundBufferIdxVol[i].bufferIdx];
  if(LoadSound(i,path,std::pow(10.f,g_SoundBufferIdxVol[i].volume/2000.f))!=ZUN_SUCCESS) {
   g_GameErrorContext.Log(TH_ERR_SOUNDPLAYER_FAILED_TO_LOAD_SOUND_FILE,path);return ZUN_ERROR;
  }
 }
 return ZUN_SUCCESS;
}
ZunResult SoundPlayer::LoadWav(const char* path){return CtrAudio::LoadMusic(path)?ZUN_SUCCESS:ZUN_ERROR;}
ZunResult SoundPlayer::LoadPos(const char*){return CtrAudio::HasLoop()?ZUN_SUCCESS:ZUN_ERROR;}
ZunResult SoundPlayer::PlayBGM(bool loop){return CtrAudio::PlayMusic(loop)?ZUN_SUCCESS:ZUN_ERROR;}
void SoundPlayer::StopBGM(){CtrAudio::StopMusic();}
void SoundPlayer::FadeOut(f32 seconds){CtrAudio::FadeMusic(seconds);}
void SoundPlayer::PlaySounds(){
 // End the simulation tick's deduplication window; playback was submitted
 // immediately on the first trigger, without waiting for the update to end.
 std::fill(std::begin(soundBuffersToPlay),std::end(soundBuffersToPlay),-1);
 CtrAudio::UpdateEffects();
}
void SoundPlayer::PlaySoundByIdx(SoundIdx id){
 if(id<0 || id>=32 || soundBuffersToPlay[id]>=0)return;
 soundBuffersToPlay[id]=id;
 CtrAudio::PlayEffect(id);
}

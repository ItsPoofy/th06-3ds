#pragma once
namespace CtrAudio {
bool Initialize();void Shutdown();
bool LoadEffect(int id,const char* path,float gain);void PlayEffect(int id);void UpdateEffects();
bool LoadMusic(const char* path);bool HasLoop();bool PlayMusic(bool loop);void StopMusic();void FadeMusic(float seconds);
}

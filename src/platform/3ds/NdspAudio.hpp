#pragma once
namespace CtrAudio {
bool Initialize();void Shutdown();
bool LoadEffect(int id,const char* path,float gain);void PlayEffect(int id);void UpdateEffects();
bool LoadMusic(const char* path);bool HasLoop();bool PlayMusic(bool loop);void StopMusic();void FadeMusic(float seconds);
// Playback position and track length in seconds. Returns false while no music plays.
bool MusicPosition(float& seconds,float& duration);
// Restarts the playing track at fraction (0 to 1) of its length, rounded down to a block.
void SeekMusic(float fraction);
// Holds the playing track in place. Seeking while paused keeps it paused.
void PauseMusic(bool pause);
bool MusicPaused();
// Whether the track loaded from path (as passed to LoadMusic) is playing.
bool MusicPlaying(const char* path);
}

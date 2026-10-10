#pragma once
namespace CtrSfx {
bool Initialize(int volume);
void StopRequests();
void Shutdown();
void BeginReload();
bool Load(int id,const char* filename,float gain);
void Request(int id);
void SetVolume(int volume);
void OnAudioFrame();
}

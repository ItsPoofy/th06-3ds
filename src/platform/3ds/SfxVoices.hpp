#pragma once
namespace CtrSfx {
bool Initialize(int volume);
void StopRequests();
// Join the mixer without releasing buffers still owned by NDSP.
void StopWorker();
void Shutdown();
void BeginReload();
bool Load(int id,const char* filename,float gain);
void Request(int id);
void SetVolume(int volume);
void OnAudioFrame();
}

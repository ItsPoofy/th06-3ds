#pragma once
#include <citro2d.h>
struct StageMenu;
namespace CtrPause {
void CaptureRun();
bool RestoreRun();
void RestartRun();
bool PreserveRetryMusic();
bool TakeRetryMusic();
bool RestartPending();
int Update(StageMenu& menu);
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

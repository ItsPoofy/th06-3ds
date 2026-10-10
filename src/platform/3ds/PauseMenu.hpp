#pragma once
#include <citro2d.h>
struct StageMenu;
namespace CtrPause {
void CaptureRun();
bool RestoreRun();
int Update(StageMenu& menu);
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

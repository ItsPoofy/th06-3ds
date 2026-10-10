#pragma once
#include <citro2d.h>
namespace CtrHud {
void DrawMenuFps(C3D_RenderTarget* top);
void SetFps(const char* text);
bool Initialize();
void Shutdown();
void Draw(C3D_Tex& scene,C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

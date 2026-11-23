#pragma once
#include <citro2d.h>

namespace CtrPracticeReturn {
bool Initialize();
void Shutdown();
void Update();
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
void DrawTransition(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

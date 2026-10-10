#pragma once
#include <citro2d.h>
namespace CtrFocusEffect {
void BeginStage();
void Update();
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
void Shutdown();
}

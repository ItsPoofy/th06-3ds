#pragma once
#include <citro2d.h>
struct MainMenu;
namespace CtrSelectionReturn {
void Begin(const MainMenu& menu,float infoX,float infoY);
void Advance();
void Reset();
bool From(int state);
float Progress();
void DrawTopScene();
void DrawWeapons();
void DrawStageChoices();
void DrawTop(C3D_RenderTarget* top);
void DrawBottom(C3D_RenderTarget* bottom);
}

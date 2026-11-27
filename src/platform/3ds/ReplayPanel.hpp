#pragma once
#include <citro2d.h>
struct MainMenu;
struct ResultScreen;
namespace CtrReplayPanel {
void Clear();
void Show(const MainMenu& menu);
bool Saving(int state);
void ShowSave(const ResultScreen& screen);
// The button a lower-screen tap on Play or Back presses, or 0. x and y are screen pixels.
int TouchButton(int x,int y);
void DrawBackground(C3D_RenderTarget* top);
void Release();
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

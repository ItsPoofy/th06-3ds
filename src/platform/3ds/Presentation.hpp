#pragma once
#include <citro2d.h>
struct MainMenu;
namespace CtrPresentation {
bool Initialize();
bool RefreshMaterial();
void Shutdown();
void ReleaseMenuAssets();
void BeginScene();
void MarkScene();
bool HasScene();
void LoadSurface(int slot,const char* path);
bool ReplacesSurface(int slot);
void CaptureMenu(const MainMenu& menu);
void TouchMenu(MainMenu& menu,int x,int y);
unsigned LogoOpacity();
unsigned SceneOpacity();
void CaptureShade(float left,float top,float right,float bottom,unsigned color,bool backgroundOnly=false);
void DrawOverlays(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
void DrawText(const char* text,float x,float y,bool selected=false,unsigned opacity=255,bool centered=false,float sx=1,float sy=1);
void DrawBackground(C3D_RenderTarget* top,C3D_RenderTarget* bottom,bool gameplay);
bool IsSelection(const MainMenu& menu);
void DrawMenu(C3D_RenderTarget* bottom);
bool ReplacesMenu(const MainMenu& menu,int vmIndex);
}

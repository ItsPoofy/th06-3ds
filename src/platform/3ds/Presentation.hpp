#pragma once
#include <citro2d.h>
struct MainMenu;
namespace CtrPresentation {
bool Initialize();
bool RefreshMaterial();
void Shutdown();
void ReleaseMenuAssets();
// Frees the practice menus' text sheets when the main menu closes.
void ReleaseMenuText();
void BeginScene();
void MarkScene();
bool HasScene();
void LoadSurface(int slot,const char* path);
void ReleaseSurface(int slot);
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
// Records and the Music Room fade their content in from their background
// and out to it. Set every frame. 1 shows the content fully.
void SetContentOpacity(float opacity);
// A translucent box under a scene list, in scene coordinates, with small
// arrows at its top and bottom edges when the list continues past them.
// Set every frame the list is shown. Two lists can show at once while one
// view replaces another.
void ShowListBox(float left,float top,float right,float bottom,bool moreAbove,bool moreBelow,float opacity=1);
void DrawListBox(C3D_RenderTarget* top);
// Loads the main menu artwork while Records or the Music Room fade out, so
// their content and background fade into it together. The main menu then
// starts on it without a second crossfade.
void PrepareReturn(const char* path);
// Covers the content with its background by the content opacity. While the
// background crossfades in, the content never shows more than it.
void DrawContentFade(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
}

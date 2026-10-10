#pragma once
#include <citro2d.h>
struct MainMenu;
namespace CtrSelection {
inline constexpr float ScoreboardX=464,ScoreboardY=240;
void BeginScene();
void BeginReturn(const MainMenu& menu);
void Draw(const MainMenu& menu);
void DrawTopScene(const MainMenu& menu);
void DrawTop(C3D_RenderTarget* top);
void DrawBottomPortraits(C3D_RenderTarget* bottom);
void Touch(MainMenu& menu,int x,int y);
void DrawTouchPanel();
void QueueScoreboard(int characterShot,int selectedStage,int stageCount,float x,float y,float scale,float opacity,bool lower=false);
}

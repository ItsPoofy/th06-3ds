#pragma once
namespace CtrMenuTypeface {
bool Initialize();
void Shutdown();
float Width(const char* text,float height=18);
void Draw(const char* text,float x,float y,float height,unsigned color,float maxWidth=320,bool centered=false);
// The Regular (0) or Custom (1) Practice label, in the difficulty labels' style.
void DrawMode(int mode,float centerX,float centerY,unsigned opacity,float scale=1);
}

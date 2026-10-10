#pragma once
#include <citro2d.h>
namespace CtrSelectionArt {
void RefreshLanguage();
bool Initialize();void Shutdown();
const char* DifficultyName(int difficulty);
unsigned DifficultyColor(int difficulty,unsigned opacity=255);
void Draw(const char* name,float x,float y,unsigned tint=0xffffffff,bool centered=false,float scale=1);
void DrawStretched(const char* name,float x,float y,float width,float height,unsigned tint=0xffffffff);
}

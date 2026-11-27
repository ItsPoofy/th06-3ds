#pragma once
namespace CtrReplayNames {
bool Prepare(const char* name,int size,int maxWidth);
void Draw(const char* name,float x,float centerY,int size,int maxWidth,unsigned color,bool centered=true);
void Release();
}

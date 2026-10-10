#pragma once
namespace CtrDialogue {
void Reset();
void StartLine(int line,const char* text,int fontHeight);
void Tick();
bool Active();
bool Revealing();
void Finish();
float Width(int line,float fullWidth);
}

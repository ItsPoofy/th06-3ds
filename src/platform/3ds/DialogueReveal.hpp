#pragma once
struct AnmVm;
namespace CtrDialogue {
inline constexpr int Capacity=3;
void BindOverflow(AnmVm& vm);
void ReleaseOverflow();
int LineCount();
void Reset();
void StartLine(int line,const char* text,int fontHeight);
void Tick();
bool Active();
bool Revealing();
bool AutoAdvanceReady();
void Finish();
float Width(int line,float fullWidth);
}

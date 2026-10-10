#pragma once
namespace CtrPractice {
bool Initialize();
void Shutdown();
void DrawPrompt(int state,unsigned alpha,float slide=0);
void DrawBottom(int selected,unsigned alpha,float slide=0);
int Count();
int UnlockedCount();
bool Available(int stage);
int Hit(int x,int y);
}

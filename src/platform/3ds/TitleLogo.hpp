#pragma once
struct MainMenu;
struct AnmVm;
namespace CtrTitleLogo {
void Reset();
void Return(MainMenu& menu);
void Tick(MainMenu& menu);
void SubtitleOffset(int index,AnmVm& vm);
}

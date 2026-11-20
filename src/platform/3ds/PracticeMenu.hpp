#pragma once
struct MainMenu;
namespace CtrPracticeMenu {
void Begin();
void ResetPresentation();
// atStart puts the cursor on Start Practice, for Return to Practice.
void OpenSetup(MainMenu& menu,bool atStart=false);
// 1 when a practice type is chosen, -1 to leave Practice.
int ChooseMode(MainMenu& menu);
// -1 when B leaves the setup for shot selection.
int Update(MainMenu& menu);
bool CanStart(const MainMenu& menu);
bool Touch(MainMenu& menu,int x,int y);
void Capture(const MainMenu& menu);
float TypeProgress();
// The Regular and Custom Practice cards, for the difficulty screen's transition.
void DrawTypeCards(float slide,unsigned alpha);
void DrawTop();
// The practice type's name and description on the top screen, for transitions.
void DrawTypeTop(float slide,unsigned alpha);
// The top-screen summary leaving to the right after B.
void DrawTopExit(float progress);
void DrawBottom();
// The setup panel leaving to the right after B, while shot selection enters.
void DrawExit(float progress);
}

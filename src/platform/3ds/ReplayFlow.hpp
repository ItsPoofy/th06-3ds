#pragma once
struct MainMenu;
struct Supervisor;
namespace CtrReplayFlow {
enum class Action { Navigate, Block, Start };
bool StageSelectionReady(const MainMenu& menu);
void MoveStage(MainMenu& menu);
Action Update(const MainMenu& menu);
bool Touch(int x,int y);
void DrawConfirmation();
void MenuReady();
bool ReturningToList();
void RestoreSelection(MainMenu& menu);
void Entered();
bool DeferExit(Supervisor& supervisor);
unsigned PlaybackSteps();
}

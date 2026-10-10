#pragma once
struct MainMenu;
namespace CtrSelectionFlow {bool Update(MainMenu& menu);bool TakeStartRequest();bool Touch(int x,int y);bool Confirming();bool ReturnDelivered();void Draw();void Reset();float ReturnProgress();bool CharacterReady(const MainMenu& menu);}

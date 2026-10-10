#pragma once
#include "Controller.hpp"
#include "Supervisor.hpp"
struct MainMenu;

namespace CtrControls {
const u8* State();
u16 Input();
const char* Name(i16 button);
ControllerMapping Defaults();
void UpdateConfig(MainMenu& menu);
bool Capturing();
void SelectByTouch();
}

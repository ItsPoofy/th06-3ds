#include "TitleLogo.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include <cmath>
namespace {int assemblyFrame=-1;}
void CtrTitleLogo::Reset() {assemblyFrame=-1;}
void CtrTitleLogo::Return(MainMenu& menu) {
 assemblyFrame=0;
 for(int i=27;i<=33;i++)menu.vm[i].pendingInterrupt=1;
}
void CtrTitleLogo::Tick(MainMenu& menu) {
 if(assemblyFrame<0)return;
 if(menu.gameState!=STATE_MAIN_MENU){assemblyFrame=-1;return;}
 // Native interrupt 1 assembles over 30 frames; interrupt 2 then moves the
 // completed logo to the left. Never interrupt the assembly on reload.
 ++assemblyFrame;
 bool assembled=true;
 for(int i=27;i<=33;i++)assembled=assembled && menu.vm[i].flags.isStopped;
 if(assemblyFrame>=30 && assembled) {
  for(int i=27;i<=33;i++)menu.vm[i].pendingInterrupt=2;
  assemblyFrame=-1;
 }
}
void CtrTitleLogo::SubtitleOffset(int index,AnmVm& vm) {
 if(index!=33 || !CtrSettings::English())return;
 // The two 256px sprites include unused width. Close the gap along their
 // rotated baseline, retaining the original positions/easing of every VM.
 vm.pos.x-=24*std::cos(vm.rotation.z);
 vm.pos.y+=24*std::sin(vm.rotation.z);
}

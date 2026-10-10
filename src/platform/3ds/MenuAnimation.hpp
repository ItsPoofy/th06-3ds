#pragma once
#include "MenuTiming.hpp"
struct AnmVm;
namespace CtrMenuAnimation {
void Advance(AnmVm& vm,CtrMenuTiming::Clock& clock,int fallbackFrames=30);
}

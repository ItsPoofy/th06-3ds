#pragma once
#include "inttypes.hpp"
namespace CtrLatchedInput {void Reset();u16 Pressed(u16 buttons);u16 Apply(u16 raw,bool gameplay,bool paused,bool automatic,bool dialogue=false);}

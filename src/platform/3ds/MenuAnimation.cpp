#include "MenuAnimation.hpp"
#include "AnmManager.hpp"
#include "AnmVm.hpp"
#include <algorithm>
#include <cstring>
namespace {
const AnmRawInstr* Next(const AnmRawInstr* instruction) {
    return reinterpret_cast<const AnmRawInstr*>(reinterpret_cast<const unsigned char*>(instruction->args)+instruction->argsCount);
}
int Argument(const AnmRawInstr* instruction,int index) {
    int value;std::memcpy(&value,&instruction->args[index],sizeof(value));return value;
}
int Duration(const AnmVm& vm,int fallback) {
    auto* instruction=vm.beginingOfScript;
    if(!instruction)return fallback;
    bool found=false;int start=0,end=0;
    for(int count=0;count<512;count++,instruction=Next(instruction)) {
        const int op=instruction->opcode;
        if(!found && op==AnmOpcode_InterruptLabel && Argument(instruction,0)==vm.pendingInterrupt) {
            found=true;start=Next(instruction)->time;
        }
        if(found) {
            const int time=instruction->time-start;
            if(op==AnmOpcode_PosTimeLinear || op==AnmOpcode_PosTimeDecel || op==AnmOpcode_PosTimeAccel)
                end=std::max(end,time+Argument(instruction,3));
            else if(op==AnmOpcode_Fade)end=std::max(end,time+Argument(instruction,1));
            else if(op==AnmOpcode_ScaleTime)end=std::max(end,time+Argument(instruction,2));
            if(op==AnmOpcode_Stop || op==AnmOpcode_StopHide || op==AnmOpcode_Exit || op==AnmOpcode_ExitHide)
                return end>0?end:std::max(time,1);
        }
        if(op==AnmOpcode_Exit || op==AnmOpcode_ExitHide)break;
    }
    return fallback;
}
}
void CtrMenuAnimation::Advance(AnmVm& vm,CtrMenuTiming::Clock& clock,int fallbackFrames) {
    if(vm.pendingInterrupt)clock.Reset(Duration(vm,fallbackFrames));
    for(int tick=clock.Ticks();tick>0;--tick)g_AnmManager->ExecuteScript(&vm);
}

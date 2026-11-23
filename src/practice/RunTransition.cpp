#include "RunTransition.hpp"
#include <algorithm>

namespace {
int entry=Practice::RunTransition::FadeFrames,exitFrame=-1;
float Smooth(float value) {
    value=std::clamp(value,0.f,1.f);
    return value*value*(3-2*value);
}
}
void Practice::RunTransition::Reset() {entry=FadeFrames;exitFrame=-1;}
void Practice::RunTransition::BeginEntry() {entry=0;exitFrame=-1;}
void Practice::RunTransition::BeginExit() {exitFrame=0;}
void Practice::RunTransition::Presented() {
    entry=std::min(entry+1,FadeFrames);
    if(exitFrame>=0)exitFrame=std::min(exitFrame+1,FadeFrames+1);
}
float Practice::RunTransition::Opacity() {
    const float incoming=1-Smooth(float(entry)/FadeFrames);
    const float outgoing=exitFrame<0?0:Smooth(float(exitFrame)/FadeFrames);
    return std::max(incoming,outgoing);
}
bool Practice::RunTransition::ExitPresented() {return exitFrame>FadeFrames;}
bool Practice::RunTransition::Active() {return entry<FadeFrames || exitFrame>=0;}

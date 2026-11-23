#include "Completion.hpp"
#include "RunTransition.hpp"
#include <algorithm>

namespace {
constexpr int FramesPerSecond=60;
bool active=false;
int frame=-1,duration=0;
using Practice::Completion::Action;
Action automatic=Action::ReturnPractice,chosen=Action::None;
float Smooth(float value) {
    value=std::clamp(value,0.f,1.f);
    return value*value*(3-2*value);
}
}
void Practice::Completion::Reset(){active=false;frame=-1;duration=0;chosen=Action::None;}
void Practice::Completion::Begin(bool repeat,Outcome outcome) {
    if(active)return;
    Reset();active=true;automatic=repeat?Action::Retry:Action::ReturnPractice;
    duration=CountdownSeconds*FramesPerSecond;
    if(repeat)duration=outcome==Outcome::SpellCleared?SpellRepeatSeconds*FramesPerSecond:0;
}
void Practice::Completion::BeginExit(Action action) {
    Reset();active=true;frame=0;automatic=chosen=action;
    RunTransition::BeginExit();
}
bool Practice::Completion::Active(){return active;}
bool Practice::Completion::CountingDown(){return active && frame>=0 && duration>0;}
bool Practice::Completion::Leaving(){return chosen!=Action::None;}
void Practice::Completion::Choose(Action action) {
    if(!CountingDown() || Leaving() || action==Action::None)return;
    chosen=action;RunTransition::BeginExit();
}
Practice::Completion::Action Practice::Completion::AutomaticAction(){return automatic;}
Practice::Completion::Action Practice::Completion::Tick(bool outroFinished) {
    if(!active)return Action::None;
    if(Leaving())return RunTransition::ExitPresented()?chosen:Action::None;
    if(frame<0) {
        if(outroFinished) {
            frame=0;
            if(!duration)BeginExit(automatic);
        }
        return Action::None;
    }
    if(++frame>=duration)Choose(automatic);
    return Action::None;
}
int Practice::Completion::SecondsRemaining(){return std::max(1,(duration-frame+FramesPerSecond-1)/FramesPerSecond);}
float Practice::Completion::TextOpacity(){return CountingDown()?Smooth(float(frame)/RunTransition::FadeFrames):0;}

#include "LatchedInput.hpp"
#include "Settings.hpp"
#include "Controller.hpp"
namespace {u16 previous=0,pressed=0;bool focus=false,fire=false;}
void CtrLatchedInput::Reset() {previous=pressed=0;focus=fire=false;}
u16 CtrLatchedInput::Pressed(u16 buttons) {return pressed&buttons;}
u16 CtrLatchedInput::Apply(u16 raw,bool gameplay,bool paused,bool automatic,bool dialogue) {
    pressed=raw&~previous;previous=raw;
    if(!gameplay || automatic) {focus=fire=false;return raw;}
    if(dialogue && !paused) {
        // Record physical confirm edges as ENTER pulses as well, so replay
        // playback reproduces presses even when fire was previously latched.
        if(pressed&TH_BUTTON_SHOOT)raw|=TH_BUTTON_ENTER;
        return raw;
    }
    if(paused)return raw;
    auto latch=[&](CtrSettings::Toggle option,u16 button,bool& state) {
        if(CtrSettings::Enabled(option)) {
            if(pressed&button)state=!state;
            raw=(raw&~button)|(state?button:0);
        } else state=false;
    };
    latch(CtrSettings::Toggle::Focus,TH_BUTTON_FOCUS,focus);
    latch(CtrSettings::Toggle::Fire,TH_BUTTON_SHOOT,fire);
    return raw;
}

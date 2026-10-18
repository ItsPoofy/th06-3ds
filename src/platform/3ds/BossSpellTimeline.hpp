#pragma once
#include <algorithm>

namespace CtrBossSpell {
// TH20 ascii.anm scripts 2/3 use 60 frames, mode 9. Its executable's
// interpolation dispatch (0x454ef0, case 9 at 0x455064) is quadratic in/out.
inline float TimerEase(float t) {
    t=std::clamp(t,0.f,1.f);
    return t<.5f?2*t*t:1-2*(1-t)*(1-t);
}
struct Timeline {
    bool active=false;
    float age=0,exitAge=30,infoAtExit=0,slideAtExit=0;
    float from=0,to=0,motionAge=60;
    float TimerOffset()const{return from+(to-from)*TimerEase(motionAge/60);}
    float InfoOpacity()const {
        // TH20 ascii.anm script 1: 90-frame delay, 30-frame linear fade.
        return active?std::clamp((age-90)/30,0.f,1.f):infoAtExit*(1-std::clamp(exitAge/30,0.f,1.f));
    }
    float InfoSlide()const {
        return active?20*(1-TimerEase((age-90)/30)):slideAtExit;
    }
    float ExitSlide()const {
        const float t=std::clamp(exitAge/30,0.f,1.f);
        return active?0:424*(1-(1-t)*(1-t));
    }
    void Begin(float destination) {
        from=TimerOffset();to=destination;motionAge=0;
        active=true;age=0;exitAge=30;infoAtExit=slideAtExit=0;
    }
    void End() {
        if(!active)return;
        infoAtExit=InfoOpacity();slideAtExit=InfoSlide();from=TimerOffset();to=0;motionAge=0;
        active=false;exitAge=0;
    }
    void Tick(float frames) {
        motionAge=std::min(60.f,motionAge+frames);
        if(active)age+=frames;else exitAge=std::min(30.f,exitAge+frames);
    }
};
}

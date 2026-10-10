#pragma once
#include <algorithm>
namespace CtrMenuTiming {
inline constexpr int Frames=18;
inline float Progress(int frame) {
    const float t=std::clamp(float(frame)/Frames,0.f,1.f);
    return t*t*(3-2*t);
}
// Run the original ANM instructions at their own cadence, distributing native
// ticks evenly across the shared duration. Short character spins stay quick.
struct Clock {
    int nativeFrames=Frames,remainder=0,remaining=0;
    void Reset(int duration){nativeFrames=std::max(duration,Frames);remainder=0;remaining=std::clamp(duration,1,Frames);}
    int Ticks(){if(remaining==0)return 1;--remaining;remainder+=nativeFrames;const int ticks=remainder/Frames;remainder%=Frames;return ticks;}
};
}

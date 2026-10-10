#pragma once
#include <cstdint>

// Keep fractional 60 Hz steps in counter units, avoiding millisecond rounding
// and avoiding drift when the timer frequency is not divisible by 60.
class FixedStepClock {
public:
    void Reset(std::uint64_t now, std::uint64_t counterFrequency) {
        previous=now;frequency=counterFrequency;pending=0;
    }
    void Advance(std::uint64_t now) {
        pending+=(now-previous)*60;
        previous=now;
    }
    bool ConsumeStep() {
        if(!frequency || pending<frequency) return false;
        pending-=frequency;
        return true;
    }
private:
    std::uint64_t previous=0,frequency=0,pending=0;
};

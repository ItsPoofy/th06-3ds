#pragma once
#include <array>
// Eight-frame crossfades retain the current mixture when expressions change
// again mid-transition, so no previously visible portrait suddenly disappears.
template<unsigned Count> struct CtrPortraitBlend {
    std::array<float,Count> weights{},start{};
    int target=0,remaining=0;
    void Reset(int expression=0) {
        weights={};weights[expression]=1;start=weights;target=expression;remaining=0;
    }
    void Select(int expression) {
        if(expression<0 || expression>=int(Count) || expression==target)return;
        start=weights;target=expression;remaining=8;
    }
    void Tick() {
        if(!remaining)return;
        const float t=(9-remaining--)/8.f,eased=t*t*(3-2*t);
        for(unsigned i=0;i<Count;i++)weights[i]=start[i]*(1-eased)+(int(i)==target?eased:0);
    }
};

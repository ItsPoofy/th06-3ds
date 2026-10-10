#pragma once
#include "MenuTiming.hpp"
template<class Snapshot> struct CtrPanelTransition {
    Snapshot current{},previous{};
    int key=-1,frame=CtrMenuTiming::Frames;
    void Reset(){key=-1;frame=CtrMenuTiming::Frames;}
    void Capture(int next,const Snapshot& snapshot) {
        if(next!=key) {
            previous=current;frame=key<0?CtrMenuTiming::Frames:0;key=next;
        } else if(frame<CtrMenuTiming::Frames)++frame;
        current=snapshot;
    }
    float Progress() const {return CtrMenuTiming::Progress(frame);}
};

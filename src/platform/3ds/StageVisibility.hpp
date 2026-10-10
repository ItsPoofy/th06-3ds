#pragma once
#include "ZunMath.hpp"
namespace CtrStageVisibility {
inline bool Visible(const ZunMatrix& clip,ZunVec3 origin,ZunVec3 size) {
    unsigned outside=0x7f;
    for(int corner=0;corner<8;corner++) {
        const auto p=clip*ZunVec4(ZunVec3(origin.x+((corner&1)?size.x:0),
            origin.y-((corner&2)?size.y:0),origin.z+((corner&4)?size.z:0)),1);
        unsigned planes=0;
        if(p.x < -p.w)planes|=1;if(p.x > p.w)planes|=2;
        if(p.y < -p.w)planes|=4;if(p.y > p.w)planes|=8;
        if(p.z < -p.w)planes|=16;if(p.z > p.w)planes|=32;
        if(p.w<=0)planes|=64;
        outside&=planes;
        if(!outside)return true;
    }
    // A box can cover the viewport with every corner outside. Reject it only
    // when all eight corners lie outside the same homogeneous clipping plane.
    return outside==0;
}
}

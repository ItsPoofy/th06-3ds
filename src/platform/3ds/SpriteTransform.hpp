#pragma once
#include "ZunMath.hpp"

namespace CtrSpriteTransform {
// Rendering-only specialization of ZunMatrix::Rotate(angle,0,0,1).
// Avoid building and multiplying a general 4x4 matrix for every bullet.
inline void RotateZ(ZunMatrix& matrix,float angle) {
    const float c=ZUN_COSF(angle),s=ZUN_SINF(angle);
    const float z=(1-c)+c;
    for(auto& column:matrix.m) {
        const float x=column[0],y=column[1];
        column[0]=c*x-s*y;
        column[1]=s*x+c*y;
        column[2]*=z;
    }
}
}

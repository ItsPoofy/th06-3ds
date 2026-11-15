#pragma once
#include "ZunMath.hpp"

namespace CtrSpriteTransform {
// The engine's centered quad has corners (+/-halfSize.x,+/-halfSize.y,0).
// Reuse its two transformed axes across all four corners.
template<class Vertex>
inline void TransformQuad(const ZunMatrix& matrix,const ZunVec3& halfSize,Vertex* vertices) {
    const ZunVec3 x(matrix.m[0][0]*halfSize.x,matrix.m[0][1]*halfSize.x,matrix.m[0][2]*halfSize.x);
    const ZunVec3 y(matrix.m[1][0]*halfSize.y,matrix.m[1][1]*halfSize.y,matrix.m[1][2]*halfSize.y);
    const ZunVec3 origin(matrix.m[3][0],matrix.m[3][1],matrix.m[3][2]);
    const ZunVec3 left=x*-1.f;
    vertices[0].position=ZunVec4((left-y)+origin,1);
    vertices[1].position=ZunVec4((x-y)+origin,1);
    vertices[2].position=ZunVec4((left+y)+origin,1);
    vertices[3].position=ZunVec4((x+y)+origin,1);
}

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

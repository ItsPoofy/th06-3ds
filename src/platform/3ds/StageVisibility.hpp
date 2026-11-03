#pragma once
#include "ZunMath.hpp"
#include <algorithm>
namespace CtrStageVisibility {
class Frustum {
    struct Plane {float x,y,z,w;};
    Plane planes[7];
public:
    explicit Frustum(const ZunMatrix& clip) {
        for(int axis=0;axis<3;axis++)for(int side=0;side<2;side++) {
            const float sign=side?-1.f:1.f;
            planes[axis*2+side]={clip.m[0][3]+sign*clip.m[0][axis],
                clip.m[1][3]+sign*clip.m[1][axis],clip.m[2][3]+sign*clip.m[2][axis],
                clip.m[3][3]+sign*clip.m[3][axis]};
        }
        planes[6]={clip.m[0][3],clip.m[1][3],clip.m[2][3],clip.m[3][3]};
    }
    bool Visible(ZunVec3 origin,ZunVec3 size) const {
        const float endX=origin.x+size.x,endY=origin.y-size.y,endZ=origin.z+size.z;
        const float lowX=std::min(origin.x,endX),highX=std::max(origin.x,endX);
        const float lowY=std::min(origin.y,endY),highY=std::max(origin.y,endY);
        const float lowZ=std::min(origin.z,endZ),highZ=std::max(origin.z,endZ);
        // Evaluate the box's furthest point along each plane normal. This is
        // the same common-plane rejection as eight projected corners, with
        // the camera planes prepared once for the entire stage draw pass.
        for(const auto& p:planes) {
            const float x=p.x*(p.x<0?lowX:highX);
            const float y=p.y*(p.y<0?lowY:highY);
            const float z=p.z*(p.z<0?lowZ:highZ);
            const float maximum=x+y+z+p.w;
            // Conservative rounding margin keeps edge-touching geometry;
            // it may draw a little extra, but never opens clipping cracks.
            const float margin=(std::abs(x)+std::abs(y)+std::abs(z)+std::abs(p.w)+1)*1.e-5f;
            if(maximum < -margin)return false;
        }
        return true;
    }
};
inline bool Visible(const ZunMatrix& clip,ZunVec3 origin,ZunVec3 size) {
    return Frustum(clip).Visible(origin,size);
}
}

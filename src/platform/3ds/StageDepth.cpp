#include "StageDepth.hpp"
#include "Stage.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace {
std::vector<unsigned> ranks;
struct Plane {
    struct Rect {float left,top,right,bottom;};
    float x,y,z;
    int layer;
    bool flat;
    std::vector<Rect> quads;
};
bool Overlaps(const Plane& a,const Plane& b) {
    constexpr float epsilon=1.e-4f;
    for(const auto& p:a.quads)for(const auto& q:b.quads)
        if(p.left+a.x<q.right+b.x-epsilon && q.left+b.x<p.right+a.x-epsilon &&
           p.top+a.y<q.bottom+b.y-epsilon && q.top+b.y<p.bottom+a.y-epsilon)return true;
    return false;
}
}
void CtrStageDepth::Prepare(const Stage& stage) {
    ranks.clear();
    std::vector<Plane> planes;
    for(auto* instance=stage.objectInstances;instance->id>=0;++instance) {
        const auto& object=*stage.objects[instance->id];
        Plane plane{instance->position.x,instance->position.y,
            object.firstQuad.position.z+instance->position.z,object.zLevel,true,{}};
        for(auto* quad=&object.firstQuad;quad->type>=0;
            quad=reinterpret_cast<const RawStageQuadBasic*>(reinterpret_cast<const char*>(quad)+quad->byteSize)) {
            const auto& vm=stage.quadVms[quad->vmIdx];
            if(quad->type!=0 || vm.rotation.x!=0 || vm.rotation.y!=0 || vm.angleVel.x!=0 || vm.angleVel.y!=0 ||
                vm.angleVel.z!=0 || quad->position.z+instance->position.z!=plane.z) {plane.flat=false;break;}
            if(!vm.sprite) {plane.flat=false;break;}
            const float width=std::abs(quad->size.x?quad->size.x:vm.sprite->widthPx*vm.scaleX);
            const float height=std::abs(quad->size.y?quad->size.y:vm.sprite->heightPx*vm.scaleY);
            const float cx=quad->position.x+((vm.flags.anchor&AnmVmAnchor_Left)?width*.5f:0);
            const float cy=quad->position.y+((vm.flags.anchor&AnmVmAnchor_Top)?height*.5f:0);
            const float c=std::abs(std::cos(vm.rotation.z)),s=std::abs(std::sin(vm.rotation.z));
            const float dx=(width*c+height*s)*.5f,dy=(height*c+width*s)*.5f;
            plane.quads.push_back({cx-dx,cy-dy,cx+dx,cy+dy});
        }
        unsigned rank=0;
        if(plane.flat) {
            for(unsigned i=0;i<planes.size();++i) {
                const auto& previous=planes[i];
                if(previous.flat && previous.layer==plane.layer && previous.z==plane.z &&
                    Overlaps(plane,previous))
                    rank=std::max(rank,ranks[i]+1);
            }
        }
        ranks.push_back(rank);
        planes.push_back(plane);
    }
}
float CtrStageDepth::Offset(unsigned instance) {
    // Bias post-projection depth only. Moving the actual world plane changes
    // its screen position at pitched camera angles and opens shoreline cracks.
    // 64 depth24 steps separate coplanar overdraw without changing screen XY.
    return instance<ranks.size()?-float(ranks[instance])*(1.f/262144):0;
}
void CtrStageDepth::Clear() {ranks.clear();}

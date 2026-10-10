#include "StageDepth.hpp"
#include "Stage.hpp"
#include <algorithm>
#include <vector>
namespace {
std::vector<unsigned> ranks;
struct Plane {
    float x,y,z,w,h;
    int layer;
    bool flat;
};
}
void CtrStageDepth::Prepare(const Stage& stage) {
    ranks.clear();
    std::vector<Plane> planes;
    for(auto* instance=stage.objectInstances;instance->id>=0;++instance) {
        const auto& object=*stage.objects[instance->id];
        Plane plane{object.position.x+instance->position.x,object.position.y+instance->position.y,
            object.firstQuad.position.z+instance->position.z,object.size.x,object.size.y,object.zLevel,true};
        for(auto* quad=&object.firstQuad;quad->type>=0;
            quad=reinterpret_cast<const RawStageQuadBasic*>(reinterpret_cast<const char*>(quad)+quad->byteSize)) {
            const auto& vm=stage.quadVms[quad->vmIdx];
            if(quad->type!=0 || vm.rotation.x!=0 || vm.rotation.y!=0 || vm.angleVel.x!=0 || vm.angleVel.y!=0 ||
                quad->position.z+instance->position.z!=plane.z) {plane.flat=false;break;}
        }
        unsigned rank=0;
        if(plane.flat) {
            for(unsigned i=0;i<planes.size();++i) {
                const auto& previous=planes[i];
                if(previous.flat && previous.layer==plane.layer && previous.z==plane.z &&
                    plane.x<previous.x+previous.w && previous.x<plane.x+plane.w &&
                    plane.y<previous.y+previous.h && previous.y<plane.y+plane.h)
                    rank=std::max(rank,ranks[i]+1);
            }
        }
        ranks.push_back(rank);
        planes.push_back(plane);
    }
}
float CtrStageDepth::Offset(unsigned instance) {
    // PICA's float24 transform can round coplanar tiled surfaces to different
    // depths. Preserve authored overdraw with exact, tiny plane separation.
    // Compute overlap order once; retain the existing sprite batching.
    return instance<ranks.size()?-float(ranks[instance])*(1.f/32):0;
}
void CtrStageDepth::Clear() {ranks.clear();}

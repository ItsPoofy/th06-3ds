#pragma once
#include "SpriteVertex.hpp"
#include <3ds.h>
#include <citro3d.h>
#include <algorithm>

namespace CtrGraphics {
class VertexBinding {
    const SpriteVertex* current=nullptr;
public:
    void Invalidate(){current=nullptr;}
    void Bind(const SpriteVertex* vertices) {
        if(current==vertices)return;
        auto* info=C3D_GetBufInfo();BufInfo_Init(info);
        BufInfo_Add(info,vertices,sizeof(SpriteVertex),3,0x210);
        current=vertices;
    }
};

template<class SourceVertex>
inline void PackQuad(SpriteVertex* output,const SourceVertex* input,u32 color) {
    const u32 rgba=(color&0xff00ff00u)|((color&255)<<16)|((color>>16)&255);
    for(unsigned i=0;i<4;i++) {
        const auto& vertex=input[i];
        output[i]={ZunVec3(vertex.position.x,vertex.position.y,vertex.position.z),vertex.textureUV,rgba};
    }
}

class QuadBatch {
    // Larger batches reuse this 48 KiB index table in consecutive chunks.
    static constexpr unsigned BatchQuads=4096;
    static_assert(BatchQuads*4<=65536);
    u16* indices=nullptr;
public:
    QuadBatch()=default;
    QuadBatch(const QuadBatch&)=delete;
    QuadBatch& operator=(const QuadBatch&)=delete;
    ~QuadBatch(){Shutdown();}
    bool Initialize() {
        indices=static_cast<u16*>(linearAlloc(BatchQuads*6*sizeof(u16)));
        if(!indices)return false;
        constexpr unsigned corners[]={0,1,2,1,2,3};
        for(unsigned quad=0;quad<BatchQuads;quad++)for(unsigned corner=0;corner<6;corner++)
            indices[quad*6+corner]=u16(quad*4+corners[corner]);
        GSPGPU_FlushDataCache(indices,BatchQuads*6*sizeof(u16));
        return true;
    }
    void Shutdown(){if(indices)linearFree(indices);indices=nullptr;}
    void Draw(const SpriteVertex* vertices,unsigned quads,VertexBinding& binding) const {
        while(quads) {
            const unsigned count=std::min(quads,BatchQuads);
            binding.Bind(vertices);
            C3D_DrawElements(GPU_TRIANGLES,count*6,C3D_UNSIGNED_SHORT,indices);
            vertices+=count*4;quads-=count;
        }
    }
};
}

#pragma once
#include <citro2d.h>
#include <cstdio>
namespace CtrAssets {
struct PackedTexture {
    C3D_Tex gpu{};Tex3DS_SubTexture sub{};bool ready=false;
    void Clear() {if(ready)C3D_TexDelete(&gpu);ready=false;}
    bool Load(const char* path) {
        Clear();FILE* f=fopen(path,"rb");if(!f)return false;
        unsigned h[5]{};
        if(fread(h,sizeof(h),1,f)!=1 || h[0]!=0x44335854 || h[3]<8 || h[4]<8 ||
           h[3]>1024 || h[4]>1024 || !h[1] || !h[2] || h[1]>h[3] || h[2]>h[4] ||
           (h[3]&(h[3]-1)) || (h[4]&(h[4]-1))) {fclose(f);return false;}
        if(!C3D_TexInit(&gpu,h[3],h[4],GPU_RGBA8)) {fclose(f);return false;}
        ready=true;const bool ok=fread(gpu.data,1,gpu.size,f)==gpu.size;fclose(f);
        if(!ok) {Clear();return false;}
        C3D_TexSetFilter(&gpu,GPU_LINEAR,GPU_LINEAR);C3D_TexSetWrap(&gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);C3D_TexFlush(&gpu);
        sub={static_cast<u16>(h[1]),static_cast<u16>(h[2]),0,1,float(h[1])/h[3],1-float(h[2])/h[4]};return true;
    }
};
}

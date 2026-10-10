#include "MenuLayer.hpp"
bool CtrMenuLayer::Initialize() {
    // Citro3D can attach only VRAM textures to a render target.
    if(!C3D_TexInitVRAM(&texture,512,256,GPU_RGBA8))return false;
    ready=true;
    C3D_TexSetFilter(&texture,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    target=C3D_RenderTargetCreateFromTex(&texture,GPU_TEXFACE_2D,0,-1);
    return target!=nullptr;
}
void CtrMenuLayer::Shutdown() {
    if(target)C3D_RenderTargetDelete(target);
    target=nullptr;
    if(ready)C3D_TexDelete(&texture);
    ready=false;
}
void CtrMenuLayer::Begin(bool clear) {
    if(clear || !visible)C3D_RenderTargetClear(target,C3D_CLEAR_COLOR,0,0);
    C3D_FrameDrawOn(target);
    visible=true;
}
void CtrMenuLayer::Draw(C3D_RenderTarget* bottom) {
    if(!visible)return;
    Tex3DS_SubTexture sub{320,240,0,240.f/256,320.f/512,0};
    C2D_SceneBegin(bottom);
    C2D_DrawImageAt({&texture,&sub},0,0,0);
}

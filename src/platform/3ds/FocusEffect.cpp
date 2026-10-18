#include "FocusEffect.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "GameplayLayout.hpp"
#include "Settings.hpp"
#include "Player.hpp"
#include "Gui.hpp"
#include "Supervisor.hpp"
#include <algorithm>
#include <cmath>
namespace {
CtrAssets::PackedTexture ring,dot;
constexpr float Pi=3.1415927f;
float frames=0;
float opacity=0;
bool Available() {
    return CtrSettings::Enabled(CtrSettings::Toggle::Hitbox) &&
        (!g_Gui.impl || !g_Gui.impl->finishedStage) &&
        (g_Player.playerState==PLAYER_STATE_ALIVE || g_Player.playerState==PLAYER_STATE_INVULNERABLE);
}
}
void CtrFocusEffect::BeginStage() {
    opacity=0;frames=0;
    if(!ring.ready)ring.Load(TH06_ASSET_ROOT "ui/focus-th20.bin");
    if(!dot.ready)dot.Load(TH06_ASSET_ROOT "ui/focus-dot.bin");
}
void CtrFocusEffect::Update() {
    if(!Available()){opacity=0;frames=0;return;}
    // Reverse from the current opacity when focus changes mid-fade.
    const float elapsed=g_Supervisor.FramerateMultiplier();
    opacity=std::clamp(opacity+(g_Player.isFocus?elapsed:-elapsed)/10.f,0.f,1.f);
    if(opacity>0)frames+=elapsed;else frames=0;
}
void CtrFocusEffect::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!ring.ready || !Available() || opacity<=0)return;
    const float fieldScale=CtrLayout::Scale();
    const float x=g_Player.positionCenter.x*fieldScale,y=g_Player.positionCenter.y*fieldScale+CtrLayout::Offset();
    // TH20 effect.anm scripts 19/18 rotate at +3/-6 degrees per game frame.
    const float angles[]={std::fmod(frames,120.f)*Pi/60.f,-std::fmod(frames,60.f)*Pi/30.f};
    const auto color=C2D_Color32(255,255,255,unsigned(255*opacity*opacity*(3-2*opacity)));
    auto draw=[&](C3D_RenderTarget* target,bool upper) {
        const float fieldTop=CtrLayout::Offset()-(upper?0:240);
        const float clipTop=std::max(0.f,fieldTop);
        const float clipBottom=std::min(240.f,fieldTop+448*fieldScale);
        if(clipTop>=clipBottom)return;
        C2D_SceneBegin(target);
        C2D_Flush();
        const float screenWidth=upper?400.f:320.f,fieldLeft=CtrLayout::Left(upper);
        // citro2d's tilted projection maps native (x,y) to (240-y,width-x).
        // Clip the seal to the playfield on both physical screens.
        C3D_SetScissor(GPU_SCISSOR_NORMAL,unsigned(std::floor(240-clipBottom)),
            unsigned(std::floor(screenWidth-fieldLeft-384*fieldScale)),
            unsigned(std::ceil(240-clipTop)),unsigned(std::ceil(screenWidth-fieldLeft)));
        const float cx=CtrLayout::Left(upper)+x,cy=y-(upper?0:240);
        for(int i=0;i<2;i++) {
            CtrNative::DrawImageRotated({&ring.gpu,&ring.sub},cx,cy,angles[i],
                color,fieldScale,fieldScale,false,true);
        }
        if(dot.ready)CtrNative::DrawImageRotated({&dot.gpu,&dot.sub},cx,cy,0,color,fieldScale,fieldScale);
        C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    };
    draw(top,true);if(!CtrLayout::Single())draw(bottom,false);
}
void CtrFocusEffect::Shutdown() {ring.Clear();dot.Clear();}

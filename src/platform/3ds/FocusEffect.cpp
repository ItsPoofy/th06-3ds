#include "FocusEffect.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "GameplayLayout.hpp"
#include "Settings.hpp"
#include "Player.hpp"
#include "Gui.hpp"
#include <algorithm>
#include <cmath>
namespace {
CtrAssets::PackedTexture ring;
constexpr float Pi=3.1415927f;
unsigned frames=0;int release=-1;
bool focused=false,visible=false;
float sizes[2]{},releaseSizes[2]{};
float EaseOut(float t) {t=std::clamp(t,0.f,1.f);return t*(2-t);}
bool Available() {
    return CtrSettings::Enabled(CtrSettings::Toggle::Hitbox) &&
        (!g_Gui.impl || !g_Gui.impl->finishedStage) &&
        (g_Player.playerState==PLAYER_STATE_ALIVE || g_Player.playerState==PLAYER_STATE_INVULNERABLE);
}
}
void CtrFocusEffect::BeginStage() {
    focused=visible=false;frames=0;release=-1;
    if(!ring.ready)ring.Load("sdmc:/3ds/th06/assets/ui/focus-th20.bin");
}
void CtrFocusEffect::Update() {
    if(!Available()){focused=visible=false;return;}
    const bool next=g_Player.isFocus;
    if(next && !focused){frames=0;release=-1;visible=true;}
    if(!next && focused){release=0;std::copy(sizes,sizes+2,releaseSizes);}
    focused=next;
    if(!visible)return;
    // effect.anm scripts 19/18: two counter-rotating copies of sprite23.
    // ANM mode 4 is quadratic ease-out; release interrupts scale alone.
    if(release>=0) {
        const float shrink=1-EaseOut(float(++release)/8);
        for(int i=0;i<2;i++)sizes[i]=releaseSizes[i]*shrink;
        if(release>=8)visible=false;
    } else {
        sizes[0]=.3f+.7f*EaseOut(frames/12.f);
        sizes[1]=2.3f-1.3f*EaseOut(frames/18.f);
    }
    ++frames;
}
void CtrFocusEffect::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!ring.ready || !Available() || !visible)return;
    const float fieldScale=CtrLayout::Scale();
    const float x=g_Player.positionCenter.x*fieldScale,y=g_Player.positionCenter.y*fieldScale+CtrLayout::Offset();
    const unsigned age=frames?frames-1:0;
    const float turn=2*Pi*EaseOut(age/18.f);
    const float angles[]={age<18?-Pi+turn:Pi+(age-18)%120*.05235988f,
                          age<18?Pi-turn:-Pi-(age-18)%60*.10471976f};
    auto draw=[&](C3D_RenderTarget* target,bool upper) {
        const float fieldTop=CtrLayout::Offset()-(upper?0:240);
        const float clipTop=std::max(0.f,fieldTop);
        const float clipBottom=std::min(240.f,fieldTop+448*fieldScale);
        if(clipTop>=clipBottom)return;
        C2D_SceneBegin(target);
        C2D_Flush();
        const float screenWidth=upper?400.f:320.f,fieldLeft=CtrLayout::Left(upper);
        // citro2d's tilted projection maps native (x,y) to (240-y,width-x).
        // Clip the expanding seal to the playfield on both physical screens.
        C3D_SetScissor(GPU_SCISSOR_NORMAL,unsigned(std::floor(240-clipBottom)),
            unsigned(std::floor(screenWidth-fieldLeft-384*fieldScale)),
            unsigned(std::ceil(240-clipTop)),unsigned(std::ceil(screenWidth-fieldLeft)));
        const float cx=CtrLayout::Left(upper)+x,cy=y-(upper?0:240);
        for(int i=0;i<2;i++) {
            const unsigned alpha=i?unsigned(255*std::min(age/6.f,1.f)):255;
            CtrNative::DrawImageRotated({&ring.gpu,&ring.sub},cx,cy,angles[i],
                C2D_Color32(255,255,255,alpha),sizes[i]*fieldScale,sizes[i]*fieldScale,false,true);
        }
        C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    };
    draw(top,true);if(!CtrLayout::Single())draw(bottom,false);
}
void CtrFocusEffect::Shutdown() {ring.Clear();}

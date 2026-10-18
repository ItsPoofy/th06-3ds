#include "SpellTitleDecoration.hpp"
#include "BossSpellLayout.hpp"
#include "CtrRenderer.hpp"
#include "AnmManager.hpp"
namespace {
GfxTextureHandle texture{},butterflies{};
void Draw(GfxTextureHandle art,float left,float right,float top,float height,int imageHeight,int textureHeight,bool mirrored,ZunColor color,int imageWidth=512) {
    if(!art || right<=left)return;
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(art);
    manager.SetVertexAttributes(VERTEX_ATTR_TEX_COORD);
    AnmVm state{};state.flags.zWriteDisable=1;state.color=(color&0xff000000)|0xffffff;
    manager.SetRenderStateForVm(&state);
    VertexTex1Xyzrhw vertices[4]{};
    for(int i=0;i<4;i++) {
        const bool end=i&1;
        vertices[i].position=ZunVec4(end?right:left,top+(i>=2?height:0.f),0,1);
        const bool uvEnd=mirrored?!end:end;
        vertices[i].textureUV=ZunVec2(uvEnd?(imageWidth-.5f)/512:.5f/512,i>=2?(imageHeight-.5f)/textureHeight:.5f/textureHeight);
    }
    manager.AddSpriteToDrawBuffer(vertices);
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
}
}
void CtrSpellTitleDecoration::Initialize(CtrRenderer& renderer) {
    u32 width=0,height=0;
    texture=renderer.LoadPreparedTexture("spell-title-underline",width,height);
    butterflies=renderer.LoadPreparedTexture("spell-title-butterflies",width,height);
}
void CtrSpellTitleDecoration::Shutdown(CtrRenderer& renderer) {
    if(texture)renderer.DeleteTexture(texture);
    if(butterflies)renderer.DeleteTexture(butterflies);
    texture={};
    butterflies={};
}
void CtrSpellTitleDecoration::Underline(float left,float right,float top,bool player,ZunColor color) {
    Draw(texture,left,right,top,4,8,8,player,color);
}
void CtrSpellTitleDecoration::BossButterflies(float right,float baseline,float zoom,ZunColor color) {
    // The original band is right anchored at its native aspect ratio. Its
    // underline sits 63 source pixels below the top of the high-res crop.
    const float scale=.5f*zoom;
    Draw(butterflies,right-CtrBossSpellLayout::BandWidth*zoom,right,
         baseline-63*scale,CtrBossSpellLayout::BandHeight*zoom,78,128,false,color,510);
}

#include "ClassicBossHud.hpp"
#include "ClassicBossLayout.hpp"
#include "CtrRenderer.hpp"
#include "Settings.hpp"
#include "GameDecorations.hpp"
#include "GameplayLayout.hpp"
#include "TextSizes.hpp"
#include "SpellTitleDecoration.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "EnemyManager.hpp"
#include "Gui.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"
#include <algorithm>
#include <cstdio>

namespace {
GfxTextureHandle texture{};
float bossAge=0,spellAge=0,exitAge=30;
bool spellActive=false,infoPending=false;
float infoTop=0,infoOpacity=0;
int bonus=0,successes=0,attempts=0,recordIndex=-1;

bool Selected() {return CtrSettings::BossBar()==CtrSettings::BossBarStyle::Classic;}

void Gradient(float left,float top,float right,float bottom,ZunColor upper,ZunColor lower) {
    if(right<=left)return;
    auto& manager=*g_AnmManager;
    manager.FlushVertexBuffer();
    VertexDiffuseXyzrhw vertices[4]{};
    for(int i=0;i<4;i++) {
        vertices[i].position=ZunVec4(i&1?right:left,i>=2?bottom:top,.495f,1);
        vertices[i].diffuse=ColorData(i>=2?lower:upper);
    }
    manager.SetVertexAttributes(VERTEX_ATTR_DIFFUSE);
    manager.SetAttributePointer(VERTEX_ARRAY_POSITION,sizeof(vertices[0]),&vertices[0].position);
    manager.SetAttributePointer(VERTEX_ARRAY_DIFFUSE,sizeof(vertices[0]),&vertices[0].diffuse);
    manager.SetDepthMask(false);manager.SetDepthFunc(DEPTH_FUNC_ALWAYS);
    manager.SetColorOp(COMPONENT_RGB,COLOR_OP_REPLACE);
    manager.SetColorOp(COMPONENT_ALPHA,COLOR_OP_REPLACE);
    g_GfxBackend->SetBlendMode(BLEND_INV_SRC_ALPHA);
    manager.BackendDrawCall();
    manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(0);manager.SetCurrentBlendMode(0xff);
    manager.SetColorOp(COMPONENT_RGB,COLOR_OP_MODULATE);
    manager.SetColorOp(COMPONENT_ALPHA,COLOR_OP_MODULATE);
    manager.SetDepthFunc(DEPTH_FUNC_LEQUAL);
}

void Glyph(int sx,int sy,int width,int height,float x,float y,float scale,ZunColor color) {
    if(!texture)return;
    auto& manager=*g_AnmManager;
    manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(texture);
    manager.SetVertexAttributes(VERTEX_ATTR_TEX_COORD);
    AnmVm state{};state.color=color;state.flags.zWriteDisable=1;
    manager.SetRenderStateForVm(&state);manager.SetDepthFunc(DEPTH_FUNC_ALWAYS);
    manager.SetColorOp(COMPONENT_RGB,COLOR_OP_MODULATE);
    manager.SetColorOp(COMPONENT_ALPHA,COLOR_OP_MODULATE);
    VertexTex1Xyzrhw vertices[4]{};
    for(int i=0;i<4;i++) {
        vertices[i].position=ZunVec4(x+(i&1?width*scale:0),y+(i>=2?height*scale:0),.495f,1);
        vertices[i].textureUV=ZunVec2((sx+(i&1?width-.5f:.5f))/512.f,(sy+(i>=2?height-.5f:.5f))/512.f);
    }
    manager.AddSpriteToDrawBuffer(vertices);
}

void History() {
    if(recordIndex<0 || recordIndex>=int(std::size(g_GameManager.catk)))return;
    const auto& record=g_GameManager.catk[recordIndex];
    successes=record.numSuccess;attempts=record.numAttempts;
}
void Snapshot() {
    const auto& spell=g_EnemyManager.spellcardInfo;
    recordIndex=spell.idx;History();
    bonus=spell.isCapturing?std::max<i32>(0,spell.captureScore+spell.captureScore*g_Gui.SpellcardSecondsRemaining()/10):0;
}
struct Ink {int left,right;};
constexpr Ink NumberInk[12]={{0,22},{5,17},{0,22},{1,21},{0,22},{1,21},{1,22},{1,22},{1,21},{1,22},{1,11},{0,13}};
int NumberIndex(char value) {return value=='/'?11:value-'0';}
float NumberAdvance(int index) {return (NumberInk[index].right-NumberInk[index].left-3)*CtrClassicBossLayout::InfoScale;}
float NumberWidth(const char* value) {
    float width=3*CtrClassicBossLayout::InfoScale;
    for(const char* c=value;*c;c++)width+=NumberAdvance(NumberIndex(*c));
    return width;
}
void Number(const char* value,float right,float top,ZunColor color) {
    float left=right-NumberWidth(value);
    for(const char* c=value;*c;c++) {
        const int index=NumberIndex(*c);
        Glyph(index*22,56,22,34,left-NumberInk[index].left*CtrClassicBossLayout::InfoScale,
              top,CtrClassicBossLayout::InfoScale,color);
        left+=NumberAdvance(index);
    }
}
float TimerScale() {return CtrLayout::Single()?CtrTextSizes::PlayfieldTop:CtrTextSizes::PlayfieldSplit;}
}

void CtrClassicBossHud::BeginStage(CtrRenderer& renderer) {
    bossAge=spellAge=0;exitAge=30;spellActive=infoPending=false;
    bonus=successes=attempts=0;recordIndex=-1;
    if(Selected() && !texture) {
        u32 width=0,height=0;
        texture=renderer.LoadPreparedTexture("boss-spell-hud",width,height);
    }
}
void CtrClassicBossHud::Shutdown(CtrRenderer& renderer) {
    if(texture)renderer.DeleteTexture(texture);
    texture={};
}
void CtrClassicBossHud::BeginSpell() {
    if(!Selected())return;
    spellActive=true;spellAge=0;exitAge=30;Snapshot();
}
void CtrClassicBossHud::EndSpell() {
    if(!Selected() || !spellActive)return;
    Snapshot();spellActive=false;exitAge=0;
}
void CtrClassicBossHud::Tick() {
    if(!Selected() || g_GameManager.isInGameMenu || g_GameManager.isTimeStopped)return;
    const float step=g_Supervisor.FramerateMultiplier();
    bossAge=g_Gui.bossPresent?std::min(30.f,bossAge+step):0;
    if(spellActive) {spellAge+=step;if(g_EnemyManager.spellcardInfo.isActive)Snapshot();}
    else {exitAge=std::min(30.f,exitAge+step);if(exitAge<30)History();}
}
void CtrClassicBossHud::DrawBar() {
    if(!Selected())return;
    using namespace CtrClassicBossLayout;
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    const unsigned alpha=g_Gui.bossUIOpacity<<24;
    const float health=std::clamp(g_Gui.bossHealthBar2,0.f,1.f);
    const float barWidth=BarWidth(TimerScale());
    Gradient(BarLeft,BarTop,BarLeft+health*barWidth,BarTop+BarHeight,alpha|0xffffff,alpha|0x202060);
    for(const auto* enemy:g_EnemyManager.bosses) {
        if(!enemy || !enemy->flags.active || !enemy->flags.isBoss || enemy->maxLife<=0)continue;
        const float fraction=SpellFraction(g_EnemyManager.spellcardInfo.isActive,enemy->lifeCallbackThreshold,enemy->maxLife);
        // TH07 ecldata1.ecl opcode 139 marks the upcoming spell's health pink.
        Gradient(BarLeft,BarTop,BarLeft+std::min(health,fraction)*barWidth,BarTop+BarHeight,alpha|0xffa0a0,alpha|0x3f2828);
        break;
    }
    const int count=std::clamp<i32>(g_Gui.eclSetLives,0,9);
    for(int i=0;i<count;i++)
        Gradient(MarkerLeft(i,count),BarTop-1,MarkerRight(i,count),BarTop+BarHeight+1,alpha|(0xffffff-i*255/9),alpha|0x202020);
    const int name=std::clamp(CtrDecorations::CurrentBossName(),0,10);
    Glyph(name%2*256,128+name/2*32,256,32,NameLeft+48*(1-bossAge/30),NameTop,NameScale,alpha|0xffffff);
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
    manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
}
void CtrClassicBossHud::DrawTimer(int seconds,ZunColor color) {
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    auto digit=g_AsciiManager.vm0;
    digit.flags.anchor=AnmVmAnchor_TopLeft;digit.flags.zWriteDisable=1;
    digit.flags.isVisible=digit.flags.flag1=1;
    digit.scaleX=digit.scaleY=TimerScale();digit.color=color;digit.posOffset=ZunVec3(0,0,0);
    seconds=std::clamp(seconds,0,99);
    for(int i=0;i<2;i++) {
        manager.SetActiveSprite(&digit,ANM_OFFSET_ASCII+'0'-0x15+(i?seconds%10:seconds/10));
        digit.pos=ZunVec3(CtrClassicBossLayout::TimerLeft(digit.scaleX)+14*i*digit.scaleX,CtrClassicBossLayout::TimerTop,.495f);
        manager.DrawNoRotation(&digit);
    }
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
    manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
}
void CtrClassicBossHud::DrawSpellcard(const AnmVm& source) {
    if(!source.sprite || source.ctrTextWidth<=0)return;
    using namespace CtrClassicBossLayout;
    const bool player=source.activeSpriteIndex==ANM_OFFSET_TEXT+6;
    const float base=CtrLayout::Single()?CtrTextSizes::SpellTop:CtrTextSizes::SpellSplit;
    auto vm=source;auto sprite=*source.sprite;
    const float width=std::min(source.ctrTextWidth,sprite.widthPx-source.ctrTextStart);
    if(width<=0)return;
    if(source.ctrTextHeight>0) {
        sprite.heightPx=source.ctrTextHeight;
        sprite.uvEnd.y=(sprite.startPixelInclusive.y+sprite.heightPx-.5f)/sprite.textureHeight;
    }
    const float titleLeft=TitleLeft(CtrDecorations::CurrentBossName());
    const float available=player?BossRight-PlayerLeft:BossRight-titleLeft;
    const float horizontal=std::min(base,available/(width*std::max(1.f,source.scaleX)));
    sprite.uvStart.x+=source.ctrTextStart/sprite.textureWidth;
    sprite.uvEnd.x=sprite.uvStart.x+width/sprite.textureWidth;sprite.widthPx=width;
    vm.sprite=&sprite;vm.scaleX=source.scaleX*horizontal;vm.scaleY=source.scaleY*base;
    vm.flags.anchor=AnmVmAnchor_TopLeft;vm.posOffset=ZunVec3(0,0,0);
    vm.pos.x=player?PlayerLeft+(source.pos.x-192):BossRight+(source.pos.x-256)-width*vm.scaleX;
    const float bossCenter=TitleTop+sprite.heightPx*base*.5f;
    const float playerCenter=PlayerBottom-sprite.heightPx*base*.5f;
    const float center=player?344+(source.pos.y-344)*(playerCenter-344)/(440-344):
        312+(source.pos.y-312)*(312-bossCenter)/(312-40);
    vm.pos.y=center-sprite.heightPx*vm.scaleY*.5f;
    if(player)CtrSpellTitleDecoration::Underline(vm.pos.x-8*source.scaleX,
        vm.pos.x+width*vm.scaleX+8*source.scaleX,vm.pos.y+sprite.heightPx*vm.scaleY+1,true,vm.color);
    else {
        vm.color=(vm.color&0xff000000)|0xffffff;
        const float bandZoom=std::min(source.scaleX,available/TitleBandWidth);
        CtrSpellTitleDecoration::BossButterflies(BossRight+(source.pos.x-256),
            vm.pos.y+(sprite.heightPx*base+1)*source.scaleY,bandZoom,vm.color);
    }
    g_AnmManager->DrawNoRotation(&vm);g_AnmManager->SetCurrentSprite(nullptr);
    if(!player) {
        infoPending=true;
        infoTop=center+sprite.heightPx*vm.scaleY*.5f+3+20*std::clamp(1-spellAge/30,0.f,1.f);
        infoOpacity=(source.color>>24)/255.f*(spellActive?1.f:1-exitAge/30);
    }
}
void CtrClassicBossHud::DrawInfo() {
    if(!Selected() || !infoPending)return;
    infoPending=false;
    if(g_Gui.HasCurrentMsgIdx() || g_Gui.impl->finishedStage || infoOpacity<=0)return;
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    const ZunColor color=(unsigned(infoOpacity*255)<<24)|0xffffff;
    using namespace CtrClassicBossLayout;
    Glyph(0,96,96,32,BonusLeft,infoTop,InfoScale,color);
    Glyph(112,96,128,32,HistoryLeft,infoTop,InfoScale,color);
    char value[16];std::snprintf(value,sizeof(value),"%d",std::clamp(bonus,0,99999999));
    Number(value,BonusRight,infoTop,color);
    std::snprintf(value,sizeof(value),"%02d/%02d",std::clamp(successes,0,99),std::clamp(attempts,0,99));
    Number(value,HistoryRight,infoTop,color);
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
    manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
}

#include "BossSpellHud.hpp"
#include "BossSpellTimeline.hpp"
#include "BossSpellLayout.hpp"
#include "BossHealthBar.hpp"
#include "CtrRenderer.hpp"
#include "GameDecorations.hpp"
#include "GameplayLayout.hpp"
#include "TextSizes.hpp"
#include "Settings.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include <algorithm>
#include <cstdio>
#include <iterator>

namespace {
GfxTextureHandle texture{};
CtrBossSpell::Timeline timeline;
float infoTop=CtrBossSpellLayout::InfoTop(0);
int bonus=0,successes=0,attempts=0;
int recordIndex=-1;
bool capturing=false;
bool infoPending=false;
unsigned infoBossAlpha=0;

void SetGlyphColor(ZunColor color) {
    AnmVm state{};state.flags.zWriteDisable=1;state.color=color;
    g_AnmManager->SetRenderStateForVm(&state);
    g_AnmManager->SetColorOp(COMPONENT_RGB,COLOR_OP_MODULATE);
    g_AnmManager->SetColorOp(COMPONENT_ALPHA,COLOR_OP_MODULATE);
}
void GlyphVertices(int sx,int sy,int w,int h,float x,float y,float scale) {
    VertexTex1Xyzrhw vertices[4]{};
    for(int i=0;i<4;i++) {
        vertices[i].position=ZunVec4(x+(i&1?w*scale:0),y+(i>=2?h*scale:0),.495f,1);
        vertices[i].textureUV=ZunVec2((sx+(i&1?w-.5f:.5f))/512.f,(sy+(i>=2?h-.5f:.5f))/512.f);
    }
    g_AnmManager->AddSpriteToDrawBuffer(vertices);
}
void Glyph(int sx,int sy,int w,int h,float x,float y,float scale,ZunColor color) {
    SetGlyphColor(color);
    GlyphVertices(sx,sy,w,h,x,y,scale);
}
struct Ink {int left,right;};
constexpr Ink SmallInk[12]={{0,22},{5,17},{0,22},{1,21},{0,22},{1,21},{1,22},{1,22},{1,21},{1,22},{1,11},{0,13}};
constexpr int OutlineOverlap=3;
int GlyphIndex(char c) {return c=='/'?11:c=='.'?10:c>='0' && c<='9'?c-'0':-1;}
float Advance(int index) {return (SmallInk[index].right-SmallInk[index].left-OutlineOverlap)*CtrBossSpellLayout::DigitScale;}
float NumberWidth(const char* text) {
    float width=OutlineOverlap*CtrBossSpellLayout::DigitScale;
    for(const char* c=text;*c;c++)if(const int index=GlyphIndex(*c);index>=0)width+=Advance(index);
    return width;
}
void NumberGlyphs(const char* text,float x,float y) {
    for(const char* c=text;*c;c++) {
        const int index=GlyphIndex(*c);
        if(index<0)continue;
        GlyphVertices(index*22,56,22,34,x-SmallInk[index].left*CtrBossSpellLayout::DigitScale,y,CtrBossSpellLayout::DigitScale);
        x+=Advance(index);
    }
}
constexpr float LabelOutline=.45f,DigitOutline=1.3f;
template<class Draw> void Outlined(ZunColor color,float x,float y,float outline,Draw draw) {
    SetGlyphColor(color&0xff000000);
    for(int row=-1;row<=1;row++)for(int column=-1;column<=1;column++)
        if(row || column)draw(x+column*outline,y+row*outline);
    SetGlyphColor(color);
    draw(x,y);
}
void Number(const char* text,float x,float y,ZunColor color) {
    Outlined(color,x,y,DigitOutline,[&](float px,float py){NumberGlyphs(text,px,py);});
}
void Label(int sx,int sy,int w,int h,float x,float y,ZunColor color) {
    Outlined(color,x,y+CtrBossSpellLayout::LabelDrop,LabelOutline,[&](float px,float py){GlyphVertices(sx,sy,w,h,px,py,CtrBossSpellLayout::LabelScale);});
}
void ReadHistory() {
    if(recordIndex>=0 && recordIndex<int(std::size(g_GameManager.catk))) {
        const auto& record=g_GameManager.catk[recordIndex];
        successes=record.numSuccess;attempts=record.numAttempts;
    }
}
void Snapshot() {
    const auto& spell=g_EnemyManager.spellcardInfo;
    capturing=spell.isCapturing;
    bonus=std::max<i32>(0,spell.captureScore+spell.captureScore*g_Gui.SpellcardSecondsRemaining()/10);
    recordIndex=spell.idx;
    ReadHistory();
}
}

bool CtrBossSpellHud::Initialize(CtrRenderer& renderer) {
    u32 width=0,height=0;
    texture=renderer.LoadPreparedTexture("boss-spell-hud",width,height);
    return true;
}
void CtrBossSpellHud::Shutdown(CtrRenderer& renderer) {
    if(texture)renderer.DeleteTexture(texture);
    texture={};
    Reset();
}
void CtrBossSpellHud::Reset(){timeline={};infoTop=CtrBossSpellLayout::InfoTop(0);bonus=successes=attempts=0;recordIndex=-1;capturing=false;}
void CtrBossSpellHud::Begin() {
    const float titleScale=CtrLayout::Single()?CtrTextSizes::SpellTop:CtrTextSizes::SpellSplit;
    const float height=g_Gui.impl->enemySpellcardName.ctrTextHeight*titleScale;
    infoTop=CtrBossSpellLayout::InfoTop(height);
    timeline.Begin(CtrBossSpellLayout::TimerDestination(height));
}
void CtrBossSpellHud::End(){if(timeline.active)Snapshot();timeline.End();}
void CtrBossSpellHud::Tick() {
    if(g_GameManager.isInGameMenu || g_GameManager.isTimeStopped)return;
    if(timeline.active && g_EnemyManager.spellcardInfo.isActive)Snapshot();
    else if(timeline.exitAge<30)ReadHistory();
    timeline.Tick(g_Supervisor.FramerateMultiplier());
}
bool CtrBossSpellHud::DrawTimer(int fallbackSeconds,ZunColor color) {
    infoPending=false;
    if(!texture || CtrSettings::BossBar()!=CtrSettings::BossBarStyle::Modern || !CtrBossHealthBar::Available())return false;
    if(g_Gui.HasCurrentMsgIdx() || g_Gui.impl->finishedStage)return true;
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    const int seconds=std::clamp(fallbackSeconds,0,99);
    auto digit=g_AsciiManager.vm0;
    digit.flags.anchor=AnmVmAnchor_TopLeft;digit.flags.zWriteDisable=1;
    digit.flags.isVisible=digit.flags.flag1=1;
    digit.scaleX=digit.scaleY=1;
    digit.color=color;digit.posOffset=ZunVec3(0,0,0);
    for(int i=0;i<2;i++) {
        manager.SetActiveSprite(&digit,ANM_OFFSET_ASCII+'0'-0x15+(i?seconds%10:seconds/10));
        digit.pos=ZunVec3(CtrBossSpellLayout::Center+(14*i-15)*digit.scaleX,
            CtrBossSpellLayout::TimerCenter+timeline.TimerOffset()-8*digit.scaleY,.495f);
        manager.DrawNoRotation(&digit);
    }
    manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(texture);
    manager.SetVertexAttributes(VERTEX_ATTR_TEX_COORD);
    const unsigned bossAlpha=color>>24;
    infoBossAlpha=bossAlpha;infoPending=true;
    const int name=std::clamp(CtrDecorations::CurrentBossName(),0,10);
    Glyph(name%2*256,128+name/2*32,256,32,36,16,.5f,(bossAlpha<<24)|0xd0ffd0);
    auto star=g_AsciiManager.vm0;
    g_AnmManager->SetActiveSprite(&star,ANM_OFFSET_FRONT+17);
    star.flags.anchor=AnmVmAnchor_TopLeft;star.flags.zWriteDisable=1;
    star.flags.isVisible=star.flags.flag1=1;
    star.scaleX=star.scaleY=.6f;star.color=(bossAlpha<<24)|0xffffff;
    for(int i=0;i<std::clamp<i32>(g_Gui.eclSetLives,0,8);i++) {
        star.pos=ZunVec3(36+12*i,32,.495f);star.posOffset=ZunVec3(0,0,0);
        g_AnmManager->DrawNoRotation(&star);
    }
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
    manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
    return true;
}
void CtrBossSpellHud::DrawInfo() {
    if(!infoPending)return;
    infoPending=false;
    const unsigned alpha=unsigned(timeline.InfoOpacity()*infoBossAlpha);
    if(!alpha || !texture)return;
    auto& manager=*g_AnmManager;
    manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
    manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(texture);
    manager.SetVertexAttributes(VERTEX_ATTR_TEX_COORD);
    const float x=timeline.ExitSlide();
    const float top=infoTop+timeline.InfoSlide();
    const ZunColor white=(alpha<<24)|0xffffff;
    char history[20],score[16];
    std::snprintf(history,sizeof(history),"%02d/%02d",std::clamp(successes,0,99),std::clamp(attempts,0,99));
    std::snprintf(score,sizeof(score),"%d",bonus);
    using namespace CtrBossSpellLayout;
    Label(0,96,96,32,BonusLeft+x,top,white);
    if(capturing)Number(score,BonusRight-NumberWidth(score)+x,top,white);
    else Label(256,96,80,32,BonusRight-80*LabelScale+x,top,white);
    Label(112,96,128,32,HistoryLeft+x,top,white);
    Number(history,HistoryRight-NumberWidth(history)+x,top,white);
    manager.FlushVertexBuffer();manager.SetCurrentSprite(nullptr);
    manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
}

#include "GameDecorations.hpp"
#include "BossCounters.hpp"
#include "BossPresentation.hpp"
#include "BossHealthBar.hpp"
#include "BossSpellHud.hpp"
#include "Settings.hpp"
#include "FocusEffect.hpp"
#include "PortraitBlend.hpp"
#include "NativeImage.hpp"
#include "GameplayLayout.hpp"
#include "SelectionArt.hpp"
#include "TextSizes.hpp"
#include "CtrRenderer.hpp"
#include "AnmManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "GameManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace {
int current=0,playerFace=0,fade=0,stage=0,playerCharacter=-1;
int bombFace=-1,bossSpellFace=-1;
int dialogueSpeaker=-1;
CtrPortraitBlend<6> playerBlend;
CtrPortraitBlend<4> bossBlend;
bool dialogue=false,mainBossSeen=false,miniboss=false;
bool releasingDialogue=false;
constexpr unsigned battleOpacity=144;
unsigned faceOpacity[2]={battleOpacity,battleOpacity};
unsigned dialogueOpacity[2]={battleOpacity,battleOpacity};
AnmVm dialogueFaces[2];
int BossName(const AnmLoadedSprite* sprite=nullptr) {
    if(sprite && sprite->sourceFileIndex>=0 && sprite->sourceFileIndex<128) {
        const auto* file=g_AnmManager->anmFiles[sprite->sourceFileIndex];
        if(file) {
            const char* name=reinterpret_cast<const char*>(file)+file->nameOffset;
            static const char* faces[]={"face03","face05","face06","face08","face09","face10","face12"};
            static const int names[]={2,4,5,7,8,9,10};
            for(int i=0;i<7;i++)if(strstr(name,faces[i]))return names[i];
        }
    }
    if(miniboss && stage==2)return 3;
    if(miniboss && stage==4)return 6;
    switch(stage) {case 1:return 2;case 2:return 4;case 3:return 5;case 4:return 7;case 5:return 8;
        case 6:return current<2?8:9;default:return current<2?7:10;}
}
bool HasTitle(int index) {return index!=3 && index!=6;}
void Name(int index,bool right,unsigned alpha,float slide=0) {
    char name[24];snprintf(name,sizeof(name),"name%d",index);
    const float width=CtrLayout::Left();
    const float x=(right?400.f-width:0.f)+slide;
    // Reuse each side's original spell-card gradient and overlay it on the lower
    // part of the portrait.  It is translucent in front.anm by design.
    const float height=HasTitle(index)?CtrLayout::NameCardHeight:36.f,y=240-height;
    CtrSelectionArt::DrawStretched(right?"name-bar":"player-name-bar",x,y,width,height,C2D_Color32(255,255,255,alpha));
    CtrSelectionArt::Draw(name,x+width*.5f,y,C2D_Color32(255,255,255,alpha),true);
}
void Counters(bool right,int lives,int bombs,unsigned alpha=255) {
    if(!CtrSettings::Enabled(CtrSettings::Toggle::QuickInfo))return;
    char values[2][12];snprintf(values[0],sizeof(values[0]),"%d",std::max(0,lives));snprintf(values[1],sizeof(values[1]),"%d",std::max(0,bombs));
    const float height=HasTitle(right?BossName():g_GameManager.character)?CtrLayout::NameCardHeight:36.f;
    const float y=240-height-20;
    const float width=44+14*(std::strlen(values[0])+std::strlen(values[1]));
    const float left=right?400-CtrLayout::Left():0;
    float x=left+(CtrLayout::Left()-width)*.5f;
    for(int i=0;i<2;i++) {
        CtrSelectionArt::Draw(i==0?"hud-life-star":"hud-bomb-star",x,y,C2D_Color32(255,255,255,alpha));x+=18;
        for(const char* digit=values[i];*digit;digit++,x+=14) {
            char label[24];snprintf(label,sizeof(label),"hud-digit%c",*digit);
            CtrSelectionArt::Draw(label,x,y,C2D_Color32(255,255,255,alpha));
        }
        x+=4;
    }
}
void Portrait(int index,bool right,unsigned alpha) {
    if(index<0 || index>=2048)return;
    const auto& sprite=g_AnmManager->sprites[index];
    if(!alpha || sprite.sourceFileIndex<0 || sprite.sourceFileIndex>=264 || sprite.widthPx<=0 || sprite.heightPx<=0)return;
    auto* gpu=static_cast<CtrRenderer*>(g_GfxBackend)->TextureImage(g_AnmManager->textures[sprite.sourceFileIndex].handle);
    if(!gpu)return;
    Tex3DS_SubTexture sub{u16(sprite.widthPx),u16(sprite.heightPx),sprite.uvStart.x,1-sprite.uvStart.y,sprite.uvEnd.x,1-sprite.uvEnd.y};
    // Wide portraits include wings beside the same half-height body frame
    // used by the other characters. Fit that frame, letting the wings crop.
    const float bodyWidth=std::min(sprite.widthPx,sprite.heightPx*.5f);
    const float scale=std::min(CtrLayout::Left()/bodyWidth,190.f/sprite.heightPx);
    const float w=sprite.widthPx*scale,h=sprite.heightPx*scale;
    // The sidebars persist throughout the fight. Native dialogue entrance,
    // listening and exit translations must never move these portraits.
    const float sideLeft=right?400-CtrLayout::Left():0;
    const float x=sideLeft+(CtrLayout::Left()-w)*.5f,y=240-h;
    CtrNative::DrawImage({gpu,&sub},x,y,C2D_Color32(255,255,255,alpha),scale,scale);
}

}
void CtrDecorations::BeginStage(int nextStage) {
    CtrBossPresentation::Reset();
    CtrBossHealthBar::Reset();
    CtrBossSpellHud::Reset();
    const bool continuing=playerCharacter==g_GameManager.character && nextStage==stage+1;
    if(continuing){playerBlend.Select(0);playerBlend.Tick();}else playerBlend.Reset();
    playerCharacter=g_GameManager.character;
    stage=nextStage;fade=0;current=playerFace=0;dialogue=mainBossSeen=miniboss=false;
    bombFace=bossSpellFace=dialogueSpeaker=-1;bossBlend.Reset();
    releasingDialogue=false;faceOpacity[0]=faceOpacity[1]=battleOpacity;
    dialogueOpacity[0]=dialogueOpacity[1]=battleOpacity;
    for(auto& vm:dialogueFaces)vm.flags.isVisible=0;
    // Reuse the engine's loaded face textures. This also follows face-file
    // replacements during Extra dialogue without stale expression images.
    CtrFocusEffect::BeginStage();
}
int CtrDecorations::CurrentBossName(){return BossName();}
void CtrDecorations::SelectPortrait(int portrait) {if(portrait>=0 && portrait<4)current=portrait;}
void CtrDecorations::PlayerSpellcard(int sprite) {bombFace=sprite-ANM_SPRITE_FACE_CHARA_START;}
void CtrDecorations::EndPlayerSpellcard() {bombFace=-1;}
void CtrDecorations::BossSpellcard(int sprite) {if(sprite>=0 && sprite<4)bossSpellFace=sprite;}
void CtrDecorations::EndBossSpellcard() {bossSpellFace=-1;}
void CtrDecorations::BeginDialogue() {
    releasingDialogue=false;dialogueSpeaker=-1;dialogueOpacity[0]=dialogueOpacity[1]=battleOpacity;
}
void CtrDecorations::DialogueSpeaker(int side) {
    if(side<0 || side>1)return;
    dialogueSpeaker=side;releasingDialogue=false;
}
void CtrDecorations::UpdateDialogue(const GuiMsgVm& message) {
    dialogue=message.currentMsgIdx>=0;
    if(!dialogue)dialogueSpeaker=-1;
    if(dialogue) {
        for(int i=0;i<2;i++)dialogueFaces[i]=message.portraits[i];
        const int p=message.portraits[0].activeSpriteIndex-ANM_SPRITE_FACE_CHARA_START;
        if(p>=0 && p<6)playerFace=p;
        if(message.portraits[1].sprite && message.portraits[1].flags.isVisible)mainBossSeen=true;
    }
    for(int i=0;i<2;i++) {
        const unsigned target=dialogue && !releasingDialogue?
            (dialogueSpeaker>=0?(dialogueSpeaker==i?255:128):dialogueOpacity[i]):battleOpacity;
        faceOpacity[i]=unsigned(std::clamp(int(target),int(faceOpacity[i])-16,int(faceOpacity[i])+16));
    }
}
void CtrDecorations::PortraitInterrupt(int side,int interrupt) {
    if(side<0 || side>1)return;
    // The native scripts label 1/3 as speaking, 4 as listening and 2/5 as
    // leaving. Read those events directly; ECL resume also occurs before the
    // boss enters and does not mean that the conversation has ended.
    if(interrupt==1 || interrupt==3 || interrupt==4) {
        releasingDialogue=false;dialogueOpacity[side]=interrupt==4?128:255;
    } else if(interrupt==2 || interrupt==5)dialogueOpacity[side]=battleOpacity;
}
void CtrDecorations::EndDialogue() {releasingDialogue=true;dialogueSpeaker=-1;}
void CtrDecorations::UpdateBoss(bool present,int portrait) {
    CtrFocusEffect::Update();
    CtrBossCounters::Observe(present);
    SelectPortrait(portrait);
    if(present && (fade==0 || mainBossSeen))miniboss=!mainBossSeen && (stage==2 || stage==4);
    const bool shown=present || CtrBossPresentation::Visible() || (dialogue && dialogueFaces[1].sprite && dialogueFaces[1].flags.isVisible);
    fade=std::clamp(fade+(shown?2:-3),0,60);
    playerBlend.Select(bombFace>=0?bombFace:playerFace);playerBlend.Tick();
    bossBlend.Select(bossSpellFace>=0?bossSpellFace:current);bossBlend.Tick();
}
void CtrDecorations::DrawPortrait(C3D_RenderTarget* top) {
    C2D_SceneBegin(top);
    {
        const unsigned alpha=faceOpacity[0];
        for(int i=0;i<6;i++)if(playerBlend.weights[i]>0)
            Portrait(ANM_SPRITE_FACE_CHARA_START+i,false,unsigned(alpha*playerBlend.weights[i]));
        if(CtrLayout::Single()){Name(g_GameManager.character,false,255);Counters(false,g_GameManager.livesRemaining+1,g_GameManager.bombsRemaining);}
    }
    if(CtrLayout::Single()) {
        CtrSelectionArt::Draw(CtrSelectionArt::DifficultyName(g_GameManager.difficulty),CtrLayout::Left()*.5f,0,CtrSelectionArt::DifficultyColor(g_GameManager.difficulty),true,CtrTextSizes::StatusDifficulty);
        char label[24];snprintf(label,sizeof(label),"Stage %d",std::clamp(int(g_GameManager.currentStage),1,7));
        CtrSelectionArt::Draw(label,400-CtrLayout::Left()*.5f,0,CtrSelectionArt::DifficultyColor(g_GameManager.difficulty),true,CtrTextSizes::StatusStage);
    }
    if(fade<=0)return;
    const unsigned alpha=faceOpacity[1]*fade/60;
    if(miniboss) {
        // Daiyousei and Koakuma have no dialogue portrait assets.
        if(CtrLayout::Single())Name(BossName(),true,255*fade/60);
    } else {
        for(int i=0;i<4;i++)if(bossBlend.weights[i]>0)
            Portrait(ANM_SPRITE_FACE_STAGE_START+i,true,unsigned(alpha*bossBlend.weights[i]));
        if(CtrLayout::Single())Name(BossName(&g_AnmManager->sprites[ANM_SPRITE_FACE_STAGE_START+current]),true,255*fade/60);
    }
    if(CtrLayout::Single())Counters(true,CtrBossCounters::Lives(),CtrBossCounters::Spells(),255*fade/60);
}
void CtrDecorations::DrawHitbox(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    CtrFocusEffect::Draw(top,bottom);
}
void CtrDecorations::Shutdown() {CtrFocusEffect::Shutdown();}

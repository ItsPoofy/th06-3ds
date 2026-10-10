#include "PlayfieldText.hpp"
#include "GameplayLayout.hpp"
#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "TextSizes.hpp"
#include "DialogueReveal.hpp"
#include "AsciiManager.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {int musicFrame=450;bool musicBlocked=false;float dialoguePanel=0;}
void CtrPlayfieldText::ResetDialoguePanel() {dialoguePanel=0;}
void CtrPlayfieldText::TickDialoguePanel(bool visible) {
    dialoguePanel=std::clamp(dialoguePanel+(visible?1.f/12:-1.f/18),0.f,1.f);
}
float CtrPlayfieldText::DialoguePanelOpacity() {return dialoguePanel*dialoguePanel*(3-2*dialoguePanel);}
float CtrPlayfieldText::DialogueTop() {return 464.f-CtrLayout::NameCardHeight/CtrLayout::Scale();}
float CtrPlayfieldText::ResultsLeft() {
    char score[16];std::snprintf(score,sizeof(score),"%8d",g_Gui.impl?g_Gui.impl->stageScore:0);
    const float columns=std::max(20,int(12+std::strlen(score)));
    return (Left+Right-(columns*14+2)*CtrTextSizes::Results)*.5f;
}
unsigned CtrPlayfieldText::Fade(int frame,int duration) {
    float t=std::clamp(std::min(frame/12.f,(duration-frame)/18.f),0.f,1.f);
    return unsigned(255*t*t*(3-2*t));
}
void CtrPlayfieldText::CueMusic() {musicFrame=0;musicBlocked=false;}
void CtrPlayfieldText::TickMusic(bool dialogueVisible) {
    musicBlocked=dialogueVisible;
    if(musicFrame>=450)return;
    if(dialogueVisible)musicFrame=120;
    else ++musicFrame;
}
void CtrPlayfieldText::QueueNotification(const ZunVec3& position,const char* text,int frame,int duration,int fadeOutFrame) {
    if(g_Gui.impl && g_Gui.impl->finishedStage)return;
    const float width=14.f*std::strlen(text)+2;
    const float size=std::min(Scale(),(Right-Left)/width);
    const auto savedColor=g_AsciiManager.color;const auto savedScale=g_AsciiManager.scale;
    const unsigned alpha=std::min(Fade(frame,duration),Fade(std::max(frame,fadeOutFrame),duration));
    g_AsciiManager.color=(savedColor&0xffffff)|(alpha<<24);
    g_AsciiManager.scale=ZunVec2(size,size);
    auto centered=position;centered.x=224-width*size*.5f;
    g_AsciiManager.AddFormatText(&centered,"%s",text);
    g_AsciiManager.color=savedColor;g_AsciiManager.scale=savedScale;
}
float CtrPlayfieldText::Scale() {
    if(g_Gui.impl && g_Gui.impl->finishedStage)return CtrTextSizes::Results;
    return CtrLayout::Single()?CtrTextSizes::PlayfieldTop:CtrTextSizes::PlayfieldSplit;
}
void CtrPlayfieldText::QueueBonus(const ZunVec3& position,int score,int frames,int fadeOutFrame) {
    char text[32];std::snprintf(text,sizeof(text),"BONUS %8d",score);
    QueueNotification(position,text,frames,BonusDuration,fadeOutFrame);
}
void CtrPlayfieldText::EnlargeAscii(ZunVec3& position,ZunVec2& scale,const char* text,bool stageResults) {
    if(stageResults) {
        // Gui positions the whole result grid using its widest score row.
        // Keep that common left edge and scale for every label and number.
        scale=ZunVec2(CtrTextSizes::Results,CtrTextSizes::Results);
        return;
    }
    int columns=0,line=0;
    for(const char* c=text;*c;c++) {
        if(*c=='\n'){columns=std::max(columns,line);line=0;}else ++line;
    }
    columns=std::max(columns,line);
    const float width=columns*14.f*scale.x;
    // Labels and numbers share a target size; pre-scaled text is not doubled.
    const float factor=std::min(std::max(1.f,Scale()/std::max(scale.x,scale.y)),(Right-Left)/std::max(1.f,width));
    const float anchor=std::clamp((position.x-float(GAME_REGION_LEFT))/std::max(1.f,float(GAME_REGION_WIDTH)-width),0.f,1.f);
    position.x-=anchor*width*(factor-1);
    scale.x*=factor;scale.y*=factor;
    position.x=std::clamp(position.x,Left,std::max(Left,Right-columns*14.f*scale.x));
    position.y=std::min(position.y,Bottom-16*scale.y);
}
void CtrPlayfieldText::Draw(const AnmVm& source,bool centered,float targetScale) {
    if(!source.sprite || source.ctrTextWidth<=0) {g_AnmManager->Draw(&source);return;}
    auto vm=source;
    auto sprite=*source.sprite;
    const float width=std::min(source.ctrTextWidth,sprite.widthPx-source.ctrTextStart);
    if(width<=0)return;
    const float factor=std::min(targetScale>0?targetScale:Scale(),(Right-Left)/std::max(1.f,width));
    const float originalLeft=source.pos.x-((source.flags.anchor&AnmVmAnchor_Left)?0:sprite.widthPx*source.scaleX*.5f);
    const float right=originalLeft+(source.ctrTextStart+width)*source.scaleX;
    sprite.uvStart.x+=source.ctrTextStart/sprite.textureWidth;
    sprite.uvEnd.x=sprite.uvStart.x+width/sprite.textureWidth;
    sprite.widthPx=width;
    vm.sprite=&sprite;vm.scaleX=vm.scaleY=factor;
    vm.flags.anchor=AnmVmAnchor_TopLeft;
    vm.pos.x=right-width*vm.scaleX;
    vm.pos.x=std::clamp(vm.pos.x,Left,std::max(Left,Right-width*vm.scaleX));
    vm.pos.y=source.pos.y-((source.flags.anchor&AnmVmAnchor_Top)?0:sprite.heightPx*source.scaleY*.5f);
    vm.pos.y=std::min(vm.pos.y,Bottom-sprite.heightPx*vm.scaleY);
    if(centered) {vm.flags.anchor=AnmVmAnchor_Center;vm.pos=source.pos;}
    // Screen-space text keeps its fade and Z rotation without inheriting the
    // perspective projection used by the 3D stage.
    g_AnmManager->Draw(&vm);
    g_AnmManager->SetCurrentSprite(nullptr);
}
void CtrPlayfieldText::DrawStageTitle(const AnmVm& source,int stage) {
    if(!source.sprite || source.ctrTextWidth<=0)return;
    const float width=std::min(source.ctrTextWidth,source.sprite->widthPx-source.ctrTextStart);
    if(width<=0)return;
    const float size=CtrLayout::Single()?1.5f:1.25f;
    // text.anm already scales stage captions by 1.5 and places them relative
    // to the playfield. Normalize that size before applying our shared font
    // size; DrawStageElements installs the full-screen GUI viewport.
    auto caption=source;
    caption.color=(source.color&0xffffffu)|(Fade(source.currentTimeInScript.current-120,240)<<24);
    caption.posOffset=ZunVec3(0,0,0);caption.rotation=ZunVec3(0,0,0);
    caption.scaleX=1;
    caption.scaleY=1;
    const float headingHeight=16*size,captionHeight=source.sprite->heightPx*size;
    const float centerX=GAME_REGION_LEFT+GAME_REGION_WIDTH*.5f;
    const float headingTop=224-(headingHeight+12+captionHeight)*.5f;
    caption.pos.x=centerX;caption.pos.y=headingTop+headingHeight+12+captionHeight*.5f;
    auto sprite=*source.sprite;
    sprite.uvStart.x+=source.ctrTextStart/sprite.textureWidth;
    sprite.uvEnd.x=sprite.uvStart.x+width/sprite.textureWidth;
    sprite.widthPx=width;
    caption.sprite=&sprite;caption.flags.anchor=AnmVmAnchor_Center;
    caption.scaleX=caption.scaleY=size;
    g_AnmManager->Draw(&caption);g_AnmManager->SetCurrentSprite(nullptr);
    char label[24];
    if(stage<6)std::snprintf(label,sizeof(label),"STAGE %d",stage);
    else std::snprintf(label,sizeof(label),"%s",stage==6?"FINAL STAGE":"EXTRA STAGE");
    const auto savedColor=g_AsciiManager.color;const auto savedScale=g_AsciiManager.scale;
    g_AsciiManager.color=(caption.color&0xff000000u)|0xffff40u;
    g_AsciiManager.scale=ZunVec2(size,size);
    ZunVec3 position(centerX-(14*std::strlen(label)+2)*size*.5f,headingTop,source.pos.z);
    g_AsciiManager.AddFormatText(&position,"%s",label);
    g_AsciiManager.color=savedColor;g_AsciiManager.scale=savedScale;
}
void CtrPlayfieldText::DrawMusicTitle(const AnmVm& source) {
    if(g_Gui.impl && g_Gui.impl->finishedStage)return;
    if(!source.sprite || source.ctrTextWidth<=0)return;
    // Cropping the native 512px right-aligned texture changes the slide's
    // off-screen padding. Show the complete notification with a uniform fade
    // at its authored cue, rather than letting individual glyphs peek in.
    const int frame=musicFrame;
    if(musicBlocked || frame<=120 || frame>=450)return;
    const float opacity=std::clamp(std::min((frame-120)/24.f,(450-frame)/24.f),0.f,1.f);
    const unsigned alpha=unsigned(255*opacity*opacity*(3-2*opacity));
    auto vm=source;auto sprite=*source.sprite;
    const float width=std::min(source.ctrTextWidth,sprite.widthPx-source.ctrTextStart);
    if(width<=0)return;
    const float factor=std::min(Scale(),352.f/std::max(1.f,width));
    sprite.uvStart.x+=source.ctrTextStart/sprite.textureWidth;
    sprite.uvEnd.x=sprite.uvStart.x+width/sprite.textureWidth;sprite.widthPx=width;
    vm.sprite=&sprite;vm.scaleX=vm.scaleY=factor;
    vm.flags.anchor=AnmVmAnchor_TopLeft;vm.posOffset=ZunVec3(0,0,0);
    vm.pos=ZunVec3(Right-width*vm.scaleX,Bottom-sprite.heightPx*vm.scaleY,source.pos.z);
    vm.color=(source.color&0xffffffu)|(alpha<<24);vm.flags.isVisible=vm.flags.flag1=1;
    g_AnmManager->DrawNoRotation(&vm);g_AnmManager->SetCurrentSprite(nullptr);
}
void CtrPlayfieldText::DrawSpellcard(const AnmVm& source,const AnmVm& background) {
    if(g_Gui.impl && g_Gui.impl->finishedStage)return;
    if(!source.sprite || source.ctrTextWidth<=0) {Draw(source);return;}
    auto vm=source;auto sprite=*source.sprite;
    const float width=std::min(source.ctrTextWidth,sprite.widthPx-source.ctrTextStart);
    if(width<=0)return;
    const float base=CtrLayout::Single()?CtrTextSizes::SpellTop:CtrTextSizes::SpellSplit;
    // Keep text.anm's zoom, 30-frame fade, vertical settling and interrupted
    // sideways exit. Cropping the canvas must preserve its authored anchors.
    const float fit=std::min(base,(Right-Left-16)/std::max(1.f,width));
    // The scripts were authored against a 256px canvas. A wider backing
    // texture must not change their entrance, exit, or final anchors.
    constexpr float halfCanvas=128;
    sprite.uvStart.x+=source.ctrTextStart/sprite.textureWidth;
    sprite.uvEnd.x=sprite.uvStart.x+width/sprite.textureWidth;sprite.widthPx=width;
    vm.sprite=&sprite;
    vm.scaleX=source.scaleX*fit;vm.scaleY=source.scaleY*fit;
    vm.flags.anchor=AnmVmAnchor_TopLeft;
    const bool player=source.activeSpriteIndex==ANM_OFFSET_TEXT+6;
    // Original canvas centers: player x192, boss x256. These constant
    // corrections retain our final bar margins without changing the path.
    if(player)
        vm.pos.x=source.pos.x-halfCanvas*source.scaleX+source.ctrTextStart*source.scaleX+
            (Left+8-(192-halfCanvas));
    else
        vm.pos.x=source.pos.x+halfCanvas*source.scaleX-width*vm.scaleX+
            (Right-8-(256+halfCanvas));
    const float restCenter=player?Bottom-4-sprite.heightPx*fit*.5f:Top+4+sprite.heightPx*fit*.5f;
    vm.pos.y=source.pos.y+(restCenter-(player?440.f:40.f))-sprite.heightPx*vm.scaleY*.5f;
    if(background.sprite) {
        auto bar=background;bar.flags.anchor=AnmVmAnchor_Center;
        bar.pos=ZunVec3(vm.pos.x+width*vm.scaleX*.5f,vm.pos.y+sprite.heightPx*vm.scaleY*.5f,source.pos.z);
        bar.posOffset=ZunVec3(0,0,0);
        const unsigned alpha=(background.color>>24)*(vm.color>>24)/255;
        bar.color=(bar.color&0xffffffu)|(alpha<<24);
        bar.scaleX=(width*vm.scaleX+16)/background.sprite->widthPx;
        bar.scaleY=(sprite.heightPx*vm.scaleY+8)/background.sprite->heightPx;
        g_AnmManager->DrawNoRotation(&bar);
    }
    g_AnmManager->DrawNoRotation(&vm);g_AnmManager->SetCurrentSprite(nullptr);
}
void CtrPlayfieldText::DrawDialogue(const AnmVm* lines) {
    float widest=1;
    for(int i=0;i<2;i++)if(lines[i].sprite)
        widest=std::max(widest,std::min(lines[i].ctrTextWidth,lines[i].sprite->widthPx));
    const float factor=std::min(Scale(),352.f/widest);
    for(int i=0;i<2;i++) {
        const auto& source=lines[i];if(!source.sprite || !source.flags.isVisible)continue;
        auto vm=source;auto sprite=*source.sprite;
        if(source.ctrTextWidth>0) {
            sprite.widthPx=std::min(source.ctrTextWidth,sprite.widthPx);
            sprite.uvEnd.x=sprite.uvStart.x+sprite.widthPx/sprite.textureWidth;
        }
        sprite.widthPx=CtrDialogue::Width(i,sprite.widthPx);
        if(sprite.widthPx<=0)continue;
        sprite.uvEnd.x=sprite.uvStart.x+sprite.widthPx/sprite.textureWidth;
        vm.sprite=&sprite;vm.flags.anchor=AnmVmAnchor_TopLeft;
        vm.pos=ZunVec3(Left,DialogueTop()+12+28*i,source.pos.z);vm.posOffset=ZunVec3(0,0,0);
        vm.scaleX=vm.scaleY=factor;
        g_AnmManager->DrawNoRotation(&vm);g_AnmManager->SetCurrentSprite(nullptr);
    }
}

#include "MenuTiming.hpp"
#include "PracticeSelection.hpp"
#include "Presentation.hpp"
#include "SelectionGeometry.hpp"
#include "TextSizes.hpp"
#include "SelectionReturn.hpp"
#include "SelectionLayout.hpp"
#include "SelectionArt.hpp"
#include "CharacterInfo.hpp"
#include "CharacterPortraits.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "AnmManager.hpp"
#include <algorithm>
namespace {
int source=-1,frame=CtrMenuTiming::Frames,character=0,cursor=0,difficulty=0;
float infoX=0,infoY=0;unsigned sceneAlpha=255;
AnmVm characters[4],weapons[2];
void DrawVm(AnmVm vm,float x,float y,float scale,unsigned alpha) {
    if(!vm.sprite || !vm.flags.isVisible)return;
    vm.flags.anchor=AnmVmAnchor_Center;vm.flags.flag1=1;vm.pos=ZunVec3(x,y,0);vm.posOffset=ZunVec3(0,0,0);
    vm.scaleX*=scale;vm.scaleY*=scale;vm.color=(vm.color&0xffffff)|(alpha<<24);
    g_AnmManager->Draw(&vm);
}
}
void CtrSelectionReturn::Begin(const MainMenu& menu,float x,float y) {
    sceneAlpha=CtrPresentation::SceneOpacity();
    source=menu.gameState;frame=0;character=g_GameManager.character;cursor=menu.cursor;
    difficulty=g_GameManager.difficulty;infoX=x;infoY=y;
    for(int i=0;i<4;i++)characters[i]=menu.vm[86+i];
    for(int i=0;i<2;i++){weapons[i]=menu.vm[92+character*2+i];weapons[i].flags.isVisible=true;}
}
void CtrSelectionReturn::Advance() {if(frame<CtrMenuTiming::Frames)++frame;}
void CtrSelectionReturn::Reset() {source=-1;frame=CtrMenuTiming::Frames;}
bool CtrSelectionReturn::From(int state) {return frame<CtrMenuTiming::Frames && source==state;}
float CtrSelectionReturn::Progress() {return CtrMenuTiming::Progress(frame);}
void CtrSelectionReturn::DrawTopScene() {
    const float p=Progress();
    if(From(STATE_CHARACTER_SELECT))for(int i=0;i<4;i++) {
        const auto& vm=characters[i];
        DrawVm(vm,CtrSelectionGeometry::X(vm,i/2)+640*p,CtrSelectionGeometry::Y(vm,i/2),CtrSelectionGeometry::Scale(i/2),
            unsigned((vm.color>>24)*(i/2==cursor?1:.188f)*(1-p)));
    }
    if(From(STATE_SHOT_SELECT)) {
        const int count=difficulty==EXTRA?1:difficulty==EASY?5:6;
        CtrSelection::QueueScoreboard(character*2+cursor,-1,count,CtrSelection::ScoreboardX+640*p,CtrSelection::ScoreboardY,CtrTextSizes::ScoreHeading,1-p);
    }
}
void CtrSelectionReturn::DrawWeapons() {
    const float p=Progress();
    if(!From(STATE_SHOT_SELECT))return;
    for(int i=0;i<2;i++){auto vm=weapons[i];vm.scaleX=vm.scaleY=1;DrawVm(vm,(character==0?400:240)+weapons[i].posOffset.x+640*p,
        190+100*i+weapons[i].posOffset.y,1.8f,unsigned((weapons[i].color>>24)*(1-p)));}
}
void CtrSelectionReturn::DrawTop(C3D_RenderTarget* top) {
    if(!From(STATE_CHARACTER_SELECT))return;
    C2D_SceneBegin(top);
    CtrCharacterInfo::DrawFull(cursor,infoX+320*Progress(),infoY,unsigned(255*(1-Progress())));
}
void CtrSelectionReturn::DrawBottom(C3D_RenderTarget* bottom) {
    const float p=Progress();const unsigned alpha=unsigned(sceneAlpha*(1-p));
    if(From(STATE_CHARACTER_SELECT)) {
        C2D_SceneBegin(bottom);
        for(int i=0;i<2;i++) {
            CtrCharacterPortraits::Draw(i,320*p,(i==cursor?255:48)*alpha/255);
            CtrCharacterInfo::DrawIdentity(i,80+160*i+320*p,188,(i==cursor?255:128)*alpha/255);
        }
    } else if(From(STATE_DIFFICULTY_SELECT)) {
        C2D_SceneBegin(bottom);
        CtrPractice::DrawPrompt(source,alpha,320*p);
        for(int i=0;i<(difficulty==EXTRA?1:4);i++) {
            const int d=difficulty==EXTRA?4:i;const float y=CtrSelectionGeometry::DifficultyY(i,difficulty==EXTRA);
            const float x=20+320*p;const unsigned shade=i==cursor?255:140;
            CtrSelectionArt::Draw("difficulty-card",x,y,C2D_Color32(shade,shade,shade,alpha));
            CtrSelectionArt::Draw(CtrSelectionArt::DifficultyName(d),x+140,y-1,CtrSelectionArt::DifficultyColor(d,(i==cursor?255:190)*alpha/255),true,CtrSelectionGeometry::DifficultyTextScale);
        }
    }
}

void CtrSelectionReturn::DrawStageChoices() {
    if(frame<CtrMenuTiming::Frames && source!=STATE_DIFFICULTY_SELECT)CtrPractice::DrawPrompt(source,unsigned(sceneAlpha*(1-Progress())),320*Progress());
    if(From(STATE_PRACTICE_LVL_SELECT))CtrPractice::DrawBottom(cursor,unsigned(sceneAlpha*(1-Progress())),(character==0?200:-200)*Progress());
}

#include "MenuTiming.hpp"
#include "PracticeSelection.hpp"
#include "SelectionGeometry.hpp"
#include "TextSizes.hpp"
#include "SelectionLayout.hpp"
#include "SelectionFlow.hpp"
#include "SelectionArt.hpp"
#include "CharacterInfo.hpp"
#include "CharacterPortraits.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "AnmManager.hpp"
#include "SoundPlayer.hpp"
#include "Presentation.hpp"
#include "AsciiManager.hpp"
#include "StageScores.hpp"
#include "SelectionReturn.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
namespace {
GameState state=STATE_STARTUP,lastState=STATE_STARTUP;int selected=0,timer=0,difficulty=0;bool extra=false,initialCharacterEntrance=true,returningDifficulty=false;float difficultySlide=0;
float infoX=200,infoY=142;int promptSource=STATE_STARTUP;
AnmVm practicePortraits[2],practiceWeapons[2];bool returningPractice=false;
constexpr float weaponScale=1.8f;
float WeaponCenter(int character) {return character==0?400.f:240.f;}
float CarouselX(const AnmVm& vm,int character) {return CtrSelectionGeometry::X(vm,character);}
float Entrance(int frames=CtrMenuTiming::Frames) {const float t=std::clamp(float(timer)/frames,0.f,1.f);return 1-(1-t)*(1-t);}
void DrawSprite(const AnmVm& source,float x,float y,float sx,float sy,bool force=false,unsigned alpha=255) {
    AnmVm vm=source;
    if(!vm.sprite || vm.sprite->sourceFileIndex<0 || vm.sprite->sourceFileIndex>=264 || !g_AnmManager->textures[vm.sprite->sourceFileIndex].handle || (!force && !vm.flags.isVisible))return;
    vm.flags.isVisible=vm.flags.flag1=1;vm.flags.anchor=AnmVmAnchor_Center;vm.pos=ZunVec3(x,y,0);
    vm.posOffset=ZunVec3(0,0,0);vm.scaleX=sx;vm.scaleY=sy;vm.color=(vm.color&0xffffff)|(alpha<<24);
    g_AnmManager->Draw(&vm);
}
void DrawVm(const MainMenu& menu,int index,float x,float y,float sx,float sy,bool force=false,unsigned alpha=255) {
    DrawSprite(menu.vm[index],x,y,sx,sy,force,alpha);
}
}
void CtrSelection::BeginScene() {state=STATE_STARTUP;}
void CtrSelection::BeginReturn(const MainMenu& menu) {CtrSelectionReturn::Begin(menu,infoX,infoY);}
void CtrSelection::QueueScoreboard(int characterShot,int selectedStage,int stageCount,
                                  float x,float y,float textScale,float opacity,bool lower) {
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    const auto savedGui=g_AsciiManager.isGui;
#ifdef __3DS__
    const auto savedLower=g_AsciiManager.lowerScreen;g_AsciiManager.lowerScreen=lower;
#endif
    g_AsciiManager.isGui=false;
    const unsigned a=unsigned(255*std::clamp(opacity,0.f,1.f));
    const bool practice=g_GameManager.isInPracticeMode;
    const bool heading=selectedStage<0;
    const int rows=practice?stageCount:CtrStageScores::TotalCount;
    const float step=heading?28.f:32.f;
    const float height=(heading?64.f:0.f)+(rows-1)*step+16*CtrTextSizes::ScoreBody;
    ZunVec3 position(x,y-height*.5f,0);
    auto line=[&](const char* text,float scale) {
        g_AsciiManager.scale=ZunVec2(scale,scale);
        // ASCII glyphs advance 14 logical pixels; include the final 16px face.
        position.x=x-(14*std::strlen(text)+2)*scale*.5f;
        g_AsciiManager.AddFormatText(&position,"%s",text);
    };
    char text[64];
    g_AsciiManager.color=(a<<24)|0x00c0f0f0;
    if(heading) {
        std::snprintf(text,sizeof(text),"%s - SHOT %c",characterShot<2?"REIMU":"MARISA",'A'+characterShot%2);
        line(text,textScale);position.y+=36;
        line(practice?"BEST STAGE":"BEST TOTAL",CtrTextSizes::ScoreBody);position.y+=28;
    }
    for(int row=0;row<rows;row++) {
        if(practice) {
            const int index=g_GameManager.difficulty==EXTRA?6:row;
            const bool selected=selectedStage<0 || row==selectedStage;
            const unsigned rowAlpha=selected?a:a/2;
            g_AsciiManager.color=(rowAlpha<<24)|(selected?0x00c0f0f0:0x0080c0c0);
            const unsigned score=index<6 && g_GameManager.difficulty<EXTRA?
                g_GameManager.pscr[characterShot][index][g_GameManager.difficulty].score:
                CtrStageScores::Get(characterShot,g_GameManager.difficulty,index);
            std::snprintf(text,sizeof(text),"STAGE %d %.9u",index+1,score);
        } else {
            const auto& run=CtrStageScores::Total(characterShot,g_GameManager.difficulty,row);
            std::snprintf(text,sizeof(text),"%d %-8.8s %.9u",row+1,run.name,run.score);
        }
        line(text,CtrTextSizes::ScoreBody);position.y+=step;
    }
    g_AsciiManager.scale=savedScale;g_AsciiManager.color=savedColor;g_AsciiManager.isGui=savedGui;
#ifdef __3DS__
    g_AsciiManager.lowerScreen=savedLower;
#endif
}
void CtrSelection::DrawTopScene(const MainMenu& menu) {
    CtrSelectionReturn::DrawTopScene();
    if(menu.gameState!=STATE_CHARACTER_SELECT && menu.gameState!=STATE_SHOT_SELECT && menu.gameState!=STATE_PRACTICE_LVL_SELECT)return;
    // Draw the original four character VMs with their native interrupt-driven
    // spin/scale animation.  A common translation centers that carousel.
    if(menu.gameState==STATE_CHARACTER_SELECT) for(int i=86;i<=89;i++) {
        const auto& vm=menu.vm[i];const int character=(i-86)/2;
        const unsigned shade=character==menu.cursor?255:48;
        DrawVm(menu,i,CarouselX(vm,character),CtrSelectionGeometry::Y(vm,character),
            vm.scaleX*CtrSelectionGeometry::Scale(character),vm.scaleY*CtrSelectionGeometry::Scale(character),false,unsigned((vm.color>>24)*shade/255));
    }
    // Interrupt 13 already moves the chosen original VM left.  Preserve that
    // motion and place the shared stage scoreboard in the right half.
    if(menu.gameState==STATE_SHOT_SELECT || menu.gameState==STATE_PRACTICE_LVL_SELECT) {
        const int chosen=g_GameManager.character;
        for(int i=86+chosen*2;i<=87+chosen*2;i++) {
            const bool stable=menu.gameState==STATE_PRACTICE_LVL_SELECT || returningPractice || CtrSelectionReturn::From(STATE_PRACTICE_LVL_SELECT);
            if(!stable)practicePortraits[i-86-chosen*2]=menu.vm[i];
            const auto& vm=stable?practicePortraits[i-86-chosen*2]:menu.vm[i];
            DrawSprite(vm,CarouselX(vm,chosen),CtrSelectionGeometry::Y(vm,chosen),
                vm.scaleX*CtrSelectionGeometry::Scale(chosen),vm.scaleY*CtrSelectionGeometry::Scale(chosen),false,vm.color>>24);
        }
        const float t=std::clamp(float(menu.stateTimer)/CtrMenuTiming::Frames,0.f,1.f),eased=menu.gameState==STATE_PRACTICE_LVL_SELECT || returningPractice || CtrSelectionReturn::From(STATE_PRACTICE_LVL_SELECT)?1:1-(1-t)*(1-t);
        const int count=g_GameManager.difficulty==EXTRA?1:g_GameManager.difficulty==EASY?5:6;
        QueueScoreboard(g_GameManager.character*2+(menu.gameState==STATE_PRACTICE_LVL_SELECT?g_GameManager.shotType:menu.cursor),-1,count,
            ScoreboardX+600*(1-eased),ScoreboardY,CtrTextSizes::ScoreHeading,eased);
    }
}
void CtrSelection::Draw(const MainMenu& menu) {
    state=menu.gameState;selected=menu.cursor;timer=menu.stateTimer;difficulty=g_GameManager.difficulty;extra=difficulty==EXTRA;
    if(state!=lastState)promptSource=lastState;
    if(state==STATE_CHARACTER_SELECT && lastState!=state)initialCharacterEntrance=lastState==STATE_DIFFICULTY_SELECT;
    if(state==STATE_DIFFICULTY_SELECT && lastState!=state)returningDifficulty=lastState==STATE_CHARACTER_SELECT;
    if(state==STATE_SHOT_SELECT && lastState!=state)returningPractice=lastState==STATE_PRACTICE_LVL_SELECT;
    if(state==STATE_CHARACTER_SELECT)returningPractice=false;
    lastState=state;
    difficultySlide=std::clamp((192-menu.vm[81+std::clamp(difficulty,0,4)].pos.x)/176.f,0.f,1.f);
    CtrSelectionReturn::DrawWeapons();
    if(state==STATE_DIFFICULTY_SELECT)return;
    const bool choosing=state==STATE_CHARACTER_SELECT;
    const int character=choosing?selected:g_GameManager.character;
    const auto& portrait=state==STATE_PRACTICE_LVL_SELECT || returningPractice?practicePortraits[0]:menu.vm[86+character*2];
    const float departure=std::clamp((448-portrait.pos.x)/128.f,0.f,1.f);
    infoX=40+CarouselX(portrait,character)*.5f-(character==0?37:45)*departure;
    infoY=(character==0?130:128)+(portrait.pos.y-128)*CtrSelectionGeometry::Scale(character)*.5f;
    if(state==STATE_PRACTICE_LVL_SELECT && timer<CtrMenuTiming::Frames) {
        const float p=Entrance(),direction=character==0?1.f:-1.f;
        for(int i=0;i<2;i++)DrawSprite(practiceWeapons[i],WeaponCenter(character)-direction*400*p+practiceWeapons[i].posOffset.x,
            190+100*i+practiceWeapons[i].posOffset.y,weaponScale,weaponScale,true,unsigned((practiceWeapons[i].color>>24)*(1-p)));
    }
    if(state==STATE_SHOT_SELECT) {
        for(int i=0;i<2;i++) {
            const auto& vm=menu.vm[92+character*2+i];
            if(!returningPractice)practiceWeapons[i]=vm;
            const float p=returningPractice?(CtrSelectionReturn::From(STATE_PRACTICE_LVL_SELECT)?CtrSelectionReturn::Progress():1.f):Entrance();
            const float slide=(1-p)*(character==0?400:-400)*(returningPractice?-1:1);
            DrawVm(menu,92+character*2+i,WeaponCenter(character)+slide+vm.posOffset.x,
                190.f+100*i+vm.posOffset.y,weaponScale,weaponScale,true,vm.color>>24);
        }
    }
}
void CtrSelection::DrawBottomPortraits(C3D_RenderTarget* bottom) {
    CtrSelectionReturn::DrawBottom(bottom);
    if(state!=STATE_CHARACTER_SELECT && state!=STATE_SHOT_SELECT && state!=STATE_PRACTICE_LVL_SELECT)return;
    C2D_SceneBegin(bottom);
    const bool choosing=state==STATE_CHARACTER_SELECT;
    const int character=choosing?selected:g_GameManager.character;
    const float enter=choosing && initialCharacterEntrance?Entrance():1;
    const float slide=(choosing && initialCharacterEntrance?320*(1-enter):0);
    for(int i=0;i<2;i++)CtrCharacterPortraits::Draw(i,slide,
        unsigned((i==character?255:48)*enter*CtrPresentation::SceneOpacity()/255));
}
void CtrSelection::DrawTop(C3D_RenderTarget* top) {
    CtrSelectionReturn::DrawTop(top);
    if(state==STATE_STARTUP)return;
    C2D_SceneBegin(top);
    const int d=state==STATE_DIFFICULTY_SELECT?(extra?4:selected):difficulty;
    const float enter=state==STATE_DIFFICULTY_SELECT && returningDifficulty?(CtrSelectionReturn::From(STATE_CHARACTER_SELECT)?CtrSelectionReturn::Progress():1):1;
    const float x=state==STATE_DIFFICULTY_SELECT?200-120*(1-enter):200-120*difficultySlide;
    const unsigned sceneOpacity=CtrPresentation::SceneOpacity();
    CtrSelectionArt::Draw(CtrSelectionArt::DifficultyName(d),x,0,CtrSelectionArt::DifficultyColor(d,unsigned(sceneOpacity)),true);
    if(state==STATE_DIFFICULTY_SELECT || (state==STATE_CHARACTER_SELECT && initialCharacterEntrance && difficultySlide<1)) {
        char name[32];snprintf(name,sizeof(name),"description%d",d);
        const unsigned alpha=state==STATE_DIFFICULTY_SELECT?unsigned(sceneOpacity*enter):unsigned(sceneOpacity*(1-difficultySlide));
        CtrSelectionArt::Draw(name,state==STATE_DIFFICULTY_SELECT?200:200-120*difficultySlide,96,C2D_Color32(255,255,255,alpha),true);
    }

    if(state==STATE_CHARACTER_SELECT || state==STATE_SHOT_SELECT || state==STATE_PRACTICE_LVL_SELECT) {
        const bool choosing=state==STATE_CHARACTER_SELECT;
        const float enter=choosing && initialCharacterEntrance?Entrance():1;
        // The complete selected-character information stays below the native
        // carousel and to the left of the per-shot scoreboard.
        CtrCharacterInfo::DrawFull(choosing?selected:g_GameManager.character,infoX,infoY,
            unsigned(sceneOpacity*enter));
    }
}
void CtrSelection::DrawTouchPanel() {
    const bool backwards=CtrSelectionReturn::From(STATE_CHARACTER_SELECT) || CtrSelectionReturn::From(STATE_SHOT_SELECT) || CtrSelectionReturn::From(STATE_PRACTICE_LVL_SELECT);
    const float progress=backwards?CtrSelectionReturn::Progress():Entrance();
    const float direction=backwards?-1.f:1.f;
    const unsigned opacity=CtrPresentation::SceneOpacity();
    const float weaponDirection=g_GameManager.character==0?200.f:-200.f;
    const float promptSlide=state==STATE_SHOT_SELECT?weaponDirection*(returningPractice?-1.f:1.f):direction*320;
    CtrPractice::DrawPrompt(state,unsigned(opacity*progress),promptSlide*(1-progress));
    if(!backwards && timer<CtrMenuTiming::Frames &&
       ((state==STATE_CHARACTER_SELECT && promptSource==STATE_DIFFICULTY_SELECT) ||
        (state==STATE_SHOT_SELECT && promptSource==STATE_CHARACTER_SELECT) ||
        (state==STATE_PRACTICE_LVL_SELECT && promptSource==STATE_SHOT_SELECT)))
        CtrPractice::DrawPrompt(promptSource,unsigned(opacity*(1-progress)),
            (state==STATE_PRACTICE_LVL_SELECT?-weaponDirection:-320.f)*progress);
    if(state==STATE_CHARACTER_SELECT || state==STATE_SHOT_SELECT || state==STATE_PRACTICE_LVL_SELECT) {
        const bool choosing=state==STATE_CHARACTER_SELECT;
        const int character=choosing?selected:g_GameManager.character;
        const float enter=choosing && initialCharacterEntrance?Entrance():1;
        const float slide=(choosing && initialCharacterEntrance?320*(1-enter):0);
        for(int i=0;i<2;i++)CtrCharacterInfo::DrawIdentity(i,80+160*i+slide,188,
            unsigned((i==character?255:128)*enter*CtrPresentation::SceneOpacity()/255));
    }
    const bool leavingDifficulty=state==STATE_CHARACTER_SELECT && initialCharacterEntrance && timer<CtrMenuTiming::Frames;
    if(state==STATE_DIFFICULTY_SELECT || leavingDifficulty) {
        const int count=extra?1:4;
        const float exit=leavingDifficulty?Entrance():0.f;
        for(int i=0;i<count;i++) {
            const int d=extra?4:i;const float y=CtrSelectionGeometry::DifficultyY(i,extra);
            const float incoming=CtrSelectionReturn::From(STATE_CHARACTER_SELECT)?-320*(1-CtrSelectionReturn::Progress()):320*(1-Entrance());
            const float x=20+(leavingDifficulty?0:incoming)-340*exit;const bool highlight=i==(leavingDifficulty?difficulty:selected);
            const unsigned alpha=unsigned(CtrPresentation::SceneOpacity()*(1-exit));
            CtrSelectionArt::Draw("difficulty-card",x,y,C2D_Color32(highlight?255:140,highlight?255:140,highlight?255:140,alpha));
            CtrSelectionArt::Draw(CtrSelectionArt::DifficultyName(d),x+140,y-1,
                CtrSelectionArt::DifficultyColor(d,(highlight?255:190)*alpha/255),true,CtrSelectionGeometry::DifficultyTextScale);
        }
    }
    if(state==STATE_PRACTICE_LVL_SELECT)CtrPractice::DrawBottom(selected,unsigned(CtrPresentation::SceneOpacity()*Entrance()),(g_GameManager.character==0?200:-200)*(1-Entrance()));
    CtrSelectionReturn::DrawStageChoices();
    CtrSelectionFlow::Draw();
}
void CtrSelection::Touch(MainMenu& menu,int x,int y) {
    if(CtrSelectionFlow::Touch(x,y))return;
    int choice=-1;
    if(menu.gameState==STATE_DIFFICULTY_SELECT && x>=20 && x<=300) {
        if(extra && y>=100 && y<100+CtrSelectionGeometry::DifficultyHeight)choice=0;
        else if(!extra && y>=40 && y<236 && (y-40)%int(CtrSelectionGeometry::DifficultyStep)<CtrSelectionGeometry::DifficultyHeight)choice=(y-40)/int(CtrSelectionGeometry::DifficultyStep);
    } else if(menu.gameState==STATE_CHARACTER_SELECT && x>=0 && x<320 && y>=0 && y<240)choice=x/160;
    else if(menu.gameState==STATE_SHOT_SELECT) {
        const float cx=WeaponCenter(g_GameManager.character)*.5f;
        for(int i=0;i<2;i++)if(std::abs(x-cx)<=128*weaponScale*.5f && std::abs(y-(95+50*i))<=24*weaponScale*.5f)choice=i;
    } else if(menu.gameState==STATE_PRACTICE_LVL_SELECT) {
        choice=CtrPractice::Hit(x,y);
    } else return;
    if(choice<0)return;
    if(menu.gameState==STATE_CHARACTER_SELECT && extra && !g_GameManager.HasReachedMaxClears(choice,0) && !g_GameManager.HasReachedMaxClears(choice,1))return;
    if(menu.gameState==STATE_SHOT_SELECT && extra && !g_GameManager.HasReachedMaxClears(g_GameManager.character,choice))return;
    if(menu.cursor!=choice)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    // Drive the same character spin interrupts used by D-pad input.
    if(menu.gameState==STATE_CHARACTER_SELECT && menu.cursor!=choice) {
        for(int i=0;i<2;i++)for(int j=0;j<2;j++)menu.vm[86+i*2+j].pendingInterrupt=i==choice?10:11;
    }
    menu.cursor=choice;g_CurFrameInput|=TH_BUTTON_SELECTMENU;g_LastFrameInput&=~TH_BUTTON_SELECTMENU;
}

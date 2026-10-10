#include "MenuTiming.hpp"
#include "SelectionFlow.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "AnmManager.hpp"
#include "SoundPlayer.hpp"
#include "Presentation.hpp"
#include "SelectionLayout.hpp"
#include "PracticeSelection.hpp"
#include "SelectionReturn.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
namespace {
bool returning=false,returnDelivered=false;int returnFrame=0,returnSource=-1;
bool active=false,closing=false,accepted=false,yes=false,tapped=false,characterQueued=false,startRequested=false;int frame=0;
float closingOpacity=1;
float Opacity() {return closing?closingOpacity*(1-CtrMenuTiming::Progress(frame)):CtrMenuTiming::Progress(frame);}
void Open() {
    active=true;closing=false;accepted=false;yes=false;frame=0;
    g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
}
void Close(bool accept) {
    if(closing)return;
    closingOpacity=Opacity();
    closing=true;accepted=accept;frame=0;
    g_SoundPlayer.PlaySoundByIdx(accept?SOUND_SELECT:SOUND_BACK);
}
}
void CtrSelectionFlow::Reset() {active=closing=tapped=characterQueued=returning=returnDelivered=startRequested=false;returnFrame=0;CtrSelectionReturn::Reset();}
bool CtrSelectionFlow::TakeStartRequest() {const bool start=startRequested;startRequested=false;return start;}
float CtrSelectionFlow::ReturnProgress() {const float t=std::clamp(float(returnFrame)/CtrMenuTiming::Frames,0.f,1.f);return returning && !returnDelivered?1-(1-t)*(1-t):0;}
bool CtrSelectionFlow::CharacterReady(const MainMenu& menu) {
    const auto& vm=menu.vm[86+menu.cursor*2];
    const bool ready=vm.flags.isVisible && std::abs(vm.scaleX)>=.999f && vm.pendingInterrupt!=9 && vm.pendingInterrupt!=10;
    if(!ready)characterQueued=true;
    return ready;
}
bool CtrSelectionFlow::Confirming() {return active;}
bool CtrSelectionFlow::ReturnDelivered() {return returnDelivered;}
bool CtrSelectionFlow::Touch(int x,int y) {
    if(returning)return true;
    if(!active)return false;
    if(!closing && frame>=4 && y>=112 && y<=158 && x>=40 && x<=280) {yes=x<160;tapped=true;}
    return true;
}
bool CtrSelectionFlow::Update(MainMenu& menu) {
    const bool selection=menu.gameState==STATE_DIFFICULTY_SELECT || menu.gameState==STATE_CHARACTER_SELECT || menu.gameState==STATE_SHOT_SELECT || menu.gameState==STATE_PRACTICE_LVL_SELECT;
    if(returnDelivered)returnDelivered=false;
    if(returning && menu.gameState==returnSource) {
        returning=false;CtrSelectionReturn::Reset();
    }
    if(returning) {CtrSelectionReturn::Advance();if(++returnFrame>=CtrMenuTiming::Frames)returning=false;else return selection;}
    if(selection && !active) {
        if(menu.stateTimer>=CtrMenuTiming::Frames && WAS_PRESSED(TH_BUTTON_RETURNMENU)) {
            CtrSelection::BeginReturn(menu);returning=true;returnDelivered=true;returnFrame=0;returnSource=menu.gameState;
            characterQueued=false;g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);return false;
        }
    }

    if(menu.gameState==STATE_CHARACTER_SELECT) {
        if(characterQueued) {
            if(WAS_PRESSED(TH_BUTTON_RETURNMENU|TH_BUTTON_LEFT|TH_BUTTON_RIGHT)) {characterQueued=false;return false;}
            if(!CharacterReady(menu))return true;
            characterQueued=false;g_CurFrameInput|=TH_BUTTON_SELECTMENU;g_LastFrameInput&=~TH_BUTTON_SELECTMENU;
        } else if(menu.stateTimer>=CtrMenuTiming::Frames && WAS_PRESSED(TH_BUTTON_SELECTMENU) && !CharacterReady(menu))return true;
        return false;
    }
    const bool canStart=(menu.gameState==STATE_SHOT_SELECT && !g_GameManager.isInPracticeMode) || (menu.gameState==STATE_PRACTICE_LVL_SELECT && CtrPractice::Available(menu.cursor));
    if(!canStart) {Reset();return false;}
    if(!active) {
        if(menu.stateTimer>=CtrMenuTiming::Frames && WAS_PRESSED(TH_BUTTON_SELECTMENU)) {Open();return true;}
        return false;
    }
    ++frame;
    if(closing) {
        if(frame>=CtrMenuTiming::Frames) {
            const bool start=accepted;Reset();
            if(start) {startRequested=true;return false;}
        }
        return true;
    }
    if(frame>=4) {
        if(WAS_PRESSED(TH_BUTTON_RETURNMENU))Close(false);
        else {
            if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP|TH_BUTTON_DOWN|TH_BUTTON_LEFT|TH_BUTTON_RIGHT)) {yes=!yes;g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);}
            if(tapped || WAS_PRESSED(TH_BUTTON_SELECTMENU))Close(yes);
        }
    }
    tapped=false;return true;
}
void CtrSelectionFlow::Draw() {
    if(!active)return;
    const float opacity=Opacity();
    const auto shade=unsigned(217*opacity);
    C2D_DrawRectSolid(0,0,0,320,240,shade<<24);
    const char* labels[]={"Are you sure?","Yes","No"};
    for(int i=0;i<3;i++) {
        const bool highlight=i==1?yes:i==2?!yes:false;
        const float emphasis=highlight?1.08f:1.f;
        CtrPresentation::DrawText(labels[i],i==0?160:i==1?100:220,
            (i==0?76:128)-(highlight?2:0),highlight,unsigned((i==0 || highlight?255:180)*opacity),true,emphasis,emphasis);
    }
}

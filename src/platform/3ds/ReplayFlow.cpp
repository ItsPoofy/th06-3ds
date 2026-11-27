#include "ReplayFlow.hpp"
#include "ReplayStageGrid.hpp"
#include "RecordHeadings.hpp"
#include "Presentation.hpp"
#include "MenuTiming.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"
#include "SoundPlayer.hpp"
#include "practice/RunTransition.hpp"
#include "utils.hpp"
#include <3ds.h>
#include <citro2d.h>
#include <algorithm>
#include <string>

namespace {
bool confirming=false,yes=false,starting=false,tapped=false;
int frame=0,exitTarget=-1;
bool returningToList=false;
std::string playedPath;
void ResetPrompt() {confirming=starting=tapped=false;yes=false;frame=0;}
}
bool CtrReplayFlow::StageSelectionReady(const MainMenu& menu) {
    return menu.gameState==STATE_REPLAY_SELECT && menu.currentReplay;
}
void CtrReplayFlow::MoveStage(MainMenu& menu) {
    if(!StageSelectionReady(menu) || confirming || starting)return;
    std::array<bool,7> present{};
    for(int stage=0;stage<7;stage++)present[stage]=menu.currentReplay->stageReplayData[stage]!=nullptr;
    const int horizontal=int(WAS_PRESSED_PERIODIC(TH_BUTTON_RIGHT))-int(WAS_PRESSED_PERIODIC(TH_BUTTON_LEFT));
    const int vertical=int(WAS_PRESSED_PERIODIC(TH_BUTTON_DOWN))-int(WAS_PRESSED_PERIODIC(TH_BUTTON_UP));
    const int next=CtrReplayStageGrid(present).Move(menu.cursor,horizontal,vertical);
    if(next!=menu.cursor){menu.cursor=next;g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);}
}
CtrReplayFlow::Action CtrReplayFlow::Update(const MainMenu& menu) {
    if(menu.gameState!=STATE_REPLAY_SELECT) {ResetPrompt();return Action::Navigate;}
    if(!StageSelectionReady(menu))return Action::Block;
    if(starting) {
        if(!Practice::RunTransition::ExitPresented())return Action::Block;
        playedPath=menu.replayFilePaths[menu.chosenReplay];
        ResetPrompt();return Action::Start;
    }
    if(!confirming) {
        if(!WAS_PRESSED(TH_BUTTON_SELECTMENU))return Action::Navigate;
        confirming=true;yes=false;frame=0;
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        return Action::Block;
    }
    if(frame<4)return Action::Block;
    if(WAS_PRESSED(TH_BUTTON_RETURNMENU)) {
        ResetPrompt();g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    } else {
        if(WAS_PRESSED_PERIODIC(TH_BUTTON_DIRECTION)) {
            yes=!yes;g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        }
        if(tapped || WAS_PRESSED(TH_BUTTON_SELECTMENU)) {
            if(yes) {starting=true;Practice::RunTransition::BeginExit();g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);}
            else {ResetPrompt();g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);}
        }
    }
    tapped=false;return Action::Block;
}
bool CtrReplayFlow::Touch(int x,int y) {
    if(!confirming)return false;
    if(!starting && frame>=4 && x>=40 && x<=280 && y>=112 && y<=164) {yes=x<160;tapped=true;}
    return true;
}
void CtrReplayFlow::DrawConfirmation() {
    if(!confirming)return;
    const float opacity=CtrMenuTiming::Progress(frame);
    frame=std::min(frame+1,CtrMenuTiming::Frames);
    C2D_DrawRectSolid(0,0,0,320,240,C2D_Color32(0,0,0,unsigned(217*opacity)));
    CtrRecordHeadings::DrawText("replay-confirm",160,78,C2D_Color32(255,255,255,unsigned(255*opacity)),true);
    for(int i=0;i<2;i++) {
        const bool selected=i==0?yes:!yes;
        const float scale=selected?1.08f:1.f;
        CtrPresentation::DrawText(i==0?"Yes":"No",i==0?100:220,128-(selected?2:0),
            selected,unsigned((selected?255:180)*opacity),true,scale,scale);
    }
}
void CtrReplayFlow::MenuReady() {
    ResetPrompt();exitTarget=-1;
    if(Practice::RunTransition::ExitPresented())Practice::RunTransition::BeginEntry();
}
bool CtrReplayFlow::ReturningToList(){return returningToList;}
void CtrReplayFlow::RestoreSelection(MainMenu& menu) {
    if(!returningToList)return;
    menu.cursor=menu.chosenReplay=0;
    for(int i=0;i<menu.replayFilesNum;i++)if(playedPath==menu.replayFilePaths[i]) {
        menu.cursor=menu.chosenReplay=i;break;
    }
    returningToList=false;
}
void CtrReplayFlow::Entered() {ResetPrompt();exitTarget=-1;Practice::RunTransition::BeginEntry();}
bool CtrReplayFlow::DeferExit(Supervisor& supervisor) {
    if(exitTarget<0) {
        const bool playing=supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER;
        const bool leaving=supervisor.curState==SUPERVISOR_STATE_MAINMENU || supervisor.curState==SUPERVISOR_STATE_MAINMENU_REPLAY;
        if(!playing || !leaving || !g_GameManager.isInReplay || g_GameManager.demoMode)return false;
        returningToList=true;
        exitTarget=supervisor.curState;Practice::RunTransition::BeginExit();
    }
    if(Practice::RunTransition::ExitPresented()) {
        supervisor.curState=exitTarget;exitTarget=-1;return false;
    }
    supervisor.curState=supervisor.wantedState;return true;
}
unsigned CtrReplayFlow::PlaybackSteps() {
    return g_Supervisor.curState==g_Supervisor.wantedState &&
        g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER &&
        g_GameManager.isInReplay && !g_GameManager.demoMode &&
        !g_GameManager.isInGameMenu && !g_GameManager.isInRetryMenu &&
        !Practice::RunTransition::Active() && (hidKeysHeld()&KEY_R)?4u:1u;
}

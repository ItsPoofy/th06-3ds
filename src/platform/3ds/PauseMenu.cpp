#include "MenuAnimation.hpp"
#include "MenuTiming.hpp"
#include "PauseMenu.hpp"
#include "Presentation.hpp"
#include "Touch.hpp"
#include "StageMenu.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"
#include "SoundPlayer.hpp"
#include "AnmManager.hpp"
#include "LatchedInput.hpp"
#include "utils.hpp"
#include <algorithm>
namespace {
enum State {Closed,Choices,ConfirmQuit,ConfirmRestart};
enum Action {Resume,Quit,Restart};
State state=Closed;Action action=Resume;
StageMenu* active=nullptr;CtrMenuTiming::Clock clocks[6];
int cursor=0,closeFrame=0;bool yes=false,pendingRestart=false,closing=false;
struct Run {int stage;Difficulty difficulty;u8 character,shot,practice;i8 lives,bombs;bool valid=false;} run;
void Close(StageMenu& menu) {
    state=Closed;active=nullptr;menu.curState=0;menu.numFrames=0;g_GameManager.isInGameMenu=0;
}
void Open(State next,StageMenu& menu) {
    state=next;menu.numFrames=0;
    for(int i=0;i<6;i++) {
        g_AnmManager->SetAndExecuteScriptIdx(&menu.menuSprites[i],i+2);
        if(i==0 || (next==Choices?i<=2:i>=3))menu.menuSprites[i].pendingInterrupt=1;
    }
}
void BeginClose(Action next,StageMenu& menu) {
    closing=true;closeFrame=0;action=next;
    for(auto& vm:menu.menuSprites)if(vm.flags.isVisible)vm.pendingInterrupt=2;
}
void Text(const char* label,float x,float y,int index,bool selected=false) {
    const auto& vm=active->menuSprites[index];if(!vm.flags.isVisible)return;
    const float sy=std::clamp(vm.scaleY/1.5f,0.f,1.2f);
    if(sy<.001f)return;
    CtrPresentation::DrawText(label,x-(selected?2:0),y-(selected?2:0),selected,unsigned(((selected || index==0 || index==3)?255:190)*std::min(sy,1.f)),true,1,1);
}
}
void CtrPause::CaptureRun() {
    run={g_GameManager.currentStage,g_GameManager.difficulty,g_GameManager.character,g_GameManager.shotType,
         g_GameManager.isInPracticeMode,g_GameManager.livesRemaining,g_GameManager.bombsRemaining,true};
    state=Closed;pendingRestart=false;closing=false;active=nullptr;CtrLatchedInput::Reset();
}
bool CtrPause::RestoreRun() {
    if(!pendingRestart || !run.valid)return false;
    pendingRestart=false;CtrLatchedInput::Reset();
    g_GameManager.currentStage=run.stage;g_GameManager.difficulty=run.difficulty;
    g_GameManager.character=run.character;g_GameManager.shotType=run.shot;
    g_GameManager.isInPracticeMode=run.practice;
    g_GameManager.livesRemaining=run.lives;g_GameManager.bombsRemaining=run.bombs;
    g_GameManager.isInReplay=0;g_GameManager.demoMode=0;
    g_Supervisor.curState=SUPERVISOR_STATE_GAMEMANAGER;
    return true;
}
int CtrPause::Update(StageMenu& menu) {
    if(!menu.curState) {
        active=&menu;cursor=0;yes=false;closing=false;menu.curState=1;Open(Choices,menu);
    }
    for(int i=0;i<6;i++)CtrMenuAnimation::Advance(menu.menuSprites[i],clocks[i],20);
    if(closing) {
        if(++closeFrame>=CtrMenuTiming::Frames) {
            const Action next=action;Close(menu);closing=false;
            if(next==Restart) {pendingRestart=true;g_Supervisor.curState=SUPERVISOR_STATE_GAMEMANAGER_REINIT;}
            else if(next==Quit)g_Supervisor.curState=SUPERVISOR_STATE_MAINMENU;
        }
        return 0;
    }
    int x=0,y=0;const bool tap=CtrTouch::Tap(x,y);
    if(menu.numFrames++<4)return 0;
    const u16 pressed=CtrLatchedInput::Pressed(TH_BUTTON_ANY);
    bool accept=(pressed&TH_BUTTON_SELECTMENU)!=0;
    if(pressed&(TH_BUTTON_MENU|TH_BUTTON_RETURNMENU)) {
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        if(state==Choices)BeginClose(Resume,menu);else {Open(Choices,menu);yes=false;}
        return 0;
    }
    if(state==Choices) {
        const int before=cursor;
        if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP))cursor=(cursor+2)%3;
        if(WAS_PRESSED_PERIODIC(TH_BUTTON_DOWN))cursor=(cursor+1)%3;
        if(tap && x>=14 && x<=306 && y>=76 && y<172) {cursor=(y-76)/32;accept=true;}
        if(cursor!=before)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        if(accept) {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
            if(cursor==0)BeginClose(Resume,menu);
            else {Open(cursor==1?ConfirmQuit:ConfirmRestart,menu);yes=false;}
        }
    } else {
        const bool before=yes;
        if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP|TH_BUTTON_DOWN|TH_BUTTON_LEFT|TH_BUTTON_RIGHT))yes=!yes;
        if(tap && y>=112 && y<=154 && x>=50 && x<=270) {yes=x<160;accept=true;}
        if(yes!=before)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        if(accept) {
            g_SoundPlayer.PlaySoundByIdx(yes?SOUND_SELECT:SOUND_BACK);
            if(!yes)Open(Choices,menu);else BeginClose(state==ConfirmRestart?Restart:Quit,menu);
        }
    }
    return 0;
}
void CtrPause::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!g_GameManager.isInGameMenu || state==Closed || !active)return;
    C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,400,240,0xd9000000);
    Text("Paused",200,108,0);
    C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,320,240,0xd9000000);
    if(state==Choices) {
        Text("Resume",160,80,1,cursor==0);
        Text("Return to Title",160,112,2,cursor==1);
        Text("Restart from Beginning",160,144,2,cursor==2);
    } else {
        Text("Are you sure?",160,76,3);
        Text("Yes",100,122,4,yes);Text("No",220,122,5,!yes);
    }
}

#include "MenuTiming.hpp"
#include "Controls.hpp"
#include "OptionsMenu.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "LatchedInput.hpp"
#include "Supervisor.hpp"
#include "MainMenu.hpp"
#include "AsciiManager.hpp"
#include "AnmManager.hpp"
#include "SoundPlayer.hpp"
#include "FileSystem.hpp"
#include "i18n.hpp"
#include <3ds.h>
#include <array>

namespace {
constexpr std::array<u32,SDL_CONTROLLER_BUTTON_MAX> keys{
    KEY_A,KEY_B,KEY_X,KEY_Y,KEY_SELECT,0,KEY_START,0,0,KEY_L,KEY_R,
    KEY_DUP,KEY_DDOWN,KEY_DLEFT,KEY_DRIGHT
};
u8 state[SDL_CONTROLLER_BUTTON_MAX];
bool capturing=false;
bool touchSelection=false;
u32 previous=0;
void Save(MainMenu& menu) {
    std::memcpy(&g_ControllerMapping,menu.controlMapping,sizeof(ControllerMapping));
    g_Supervisor.cfg.controllerMapping=g_ControllerMapping;
    FileSystem::WriteDataToFile(TH_CONFIG_FILE,&g_Supervisor.cfg,sizeof(g_Supervisor.cfg));
    menu.gameState=STATE_OPTIONS;menu.stateTimer=0;menu.cursor=CtrOptions::ControlsRow();
    for(auto& vm:menu.vm) vm.pendingInterrupt=3;
    capturing=false;g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
}
}
const u8* CtrControls::State() {
    const u32 held=hidKeysHeld();
    for(unsigned i=0;i<keys.size();i++) state[i]=(held&keys[i])?0x80:0;
    return state;
}
ControllerMapping CtrControls::Defaults() {
    return {SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_X};
}
const char* CtrControls::Name(i16 button) {
    static const char* names[]={"A","B","X","Y","SELECT","UNBOUND","START","UNBOUND","UNBOUND",
        "L","R","D-PAD UP","D-PAD DOWN","D-PAD LEFT","D-PAD RIGHT"};
    return button>=0 && button<15?names[button]:"UNBOUND";
}
u16 CtrControls::Input() {
    u16 result=0;
    const auto* held=State();
    const auto& mapping=g_Supervisor.cfg.controllerMapping;
    const i16 bindings[]={mapping.shootButton,mapping.bombButton,mapping.focusButton,mapping.menuButton,
        mapping.upButton,mapping.downButton,mapping.leftButton,mapping.rightButton,mapping.skipButton};
    for(unsigned i=0;i<9;i++)
        if(bindings[i]>=0 && bindings[i]<(int)keys.size() && held[bindings[i]]) result|=1<<i;
    circlePosition stick;hidCircleRead(&stick);
    if(stick.dy>40) result|=TH_BUTTON_UP;
    if(stick.dy < -40) result|=TH_BUTTON_DOWN;
    if(stick.dx < -40) result|=TH_BUTTON_LEFT;
    if(stick.dx>40) result|=TH_BUTTON_RIGHT;
    const bool gameplay=g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER || g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER_REINIT;
    result=CtrLatchedInput::Apply(result,gameplay,g_GameManager.isInGameMenu || g_GameManager.isInRetryMenu,g_GameManager.isInReplay || g_GameManager.demoMode,g_Gui.impl && g_Gui.impl->msg.currentMsgIdx>=0);
    // Fixed menu navigation keeps rebinding usable even when movement is unbound.
    if(g_Supervisor.wantedState!=SUPERVISOR_STATE_GAMEMANAGER &&
       g_Supervisor.wantedState!=SUPERVISOR_STATE_GAMEMANAGER_REINIT) {
        const u32 raw=hidKeysHeld();
        if(raw&KEY_DUP) result|=TH_BUTTON_UP;
        if(raw&KEY_DDOWN) result|=TH_BUTTON_DOWN;
        if(raw&KEY_DLEFT) result|=TH_BUTTON_LEFT;
        if(raw&KEY_DRIGHT) result|=TH_BUTTON_RIGHT;
        if(raw&KEY_A) result|=TH_BUTTON_ENTER;
        if(raw&KEY_B) result|=TH_BUTTON_MENU;
    }
    return result;
}
void CtrControls::UpdateConfig(MainMenu& menu) {
    const u32 raw=hidKeysHeld(),pressed=(raw&~previous)|(touchSelection?KEY_A:0);previous=raw;touchSelection=false;
    if(menu.stateTimer<CtrMenuTiming::Frames) {capturing=false;return;}
    if(capturing) {
        for(unsigned i=0;i<keys.size();i++) if(pressed&keys[i]) {
            menu.controlMapping[menu.cursor]=i;capturing=false;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);break;
        }
    } else {
        MainMenu::MoveCursor(&menu,11);
        if(pressed&KEY_B) Save(menu);
        else if(pressed&KEY_A) {
            if(menu.cursor<9) capturing=true;
            else if(menu.cursor==9) {
                const auto defaults=Defaults();std::memcpy(menu.controlMapping,&defaults,sizeof(defaults));
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
            } else Save(menu);
        } else if((pressed&KEY_SELECT) && menu.cursor<9) menu.controlMapping[menu.cursor]=-1;
    }
}
bool CtrControls::Capturing() {return capturing;}
void CtrControls::SelectByTouch() {if(!capturing)touchSelection=true;}

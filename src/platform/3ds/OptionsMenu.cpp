#include "MenuTiming.hpp"
#include "OptionsMenu.hpp"
#include "Settings.hpp"
#include "BossHealthBar.hpp"
#include "SfxVoices.hpp"
#include "SelectionArt.hpp"
#include "Presentation.hpp"
#include "DisplaySettings.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "SoundPlayer.hpp"
#include "FileSystem.hpp"
#include "i18n.hpp"
#include "utils.hpp"
namespace {
int page=-1;bool reloadArtwork=false;
enum class Item {Lives,Bombs,Collect,Fire,Focus,Keys,Screen,Alignment,Hitbox,QuickInfo,Fps,BossBar,Bgm,Sfx,Sound,Language,Graphics,Sprites,Reset,Back};
constexpr Item gameplay[]={Item::Lives,Item::Bombs,Item::Collect,Item::Back};
constexpr Item controls[]={Item::Fire,Item::Focus,Item::Keys,Item::Back};
constexpr Item display[]={Item::Screen,Item::Alignment,Item::Hitbox,Item::QuickInfo,Item::BossBar,Item::Fps,Item::Back};
constexpr Item audio[]={Item::Bgm,Item::Sfx,Item::Sound,Item::Back};
constexpr Item content[]={Item::Language,Item::Graphics,Item::Sprites,Item::Reset,Item::Back};
struct Page {const char* heading;const Item* items;int count;};
constexpr Page pages[]={{"Gameplay",gameplay,4},{"Controls",controls,4},{"Display",display,7},{"Audio",audio,4},{"Content",content,5}};
Item At(int row) {return pages[page].items[row];}
bool ReloadSounds() {
 if(g_SoundPlayer.audioDev) {
  CtrSfx::BeginReload();
  if(g_SoundPlayer.InitSoundBuffers()!=ZUN_SUCCESS)return false;
  if(!CtrSfx::Initialize(CtrSettings::SfxVolume()))return false;
 }
 return true;
}
}
int CtrOptions::Page() {return page;}
void CtrOptions::Open() {page=-1;}
int CtrOptions::PageCount() {return sizeof(pages)/sizeof(pages[0]);}
int CtrOptions::ControlsRow() {for(int i=0;i<RowCount();i++)if(At(i)==Item::Keys)return i;return 0;}
int CtrOptions::RowCount() {return page<0?PageCount()+1:pages[page].count;}
const char* CtrOptions::Heading(int index) {return index<0?"Options":pages[index].heading;}
const char* CtrOptions::Label(int row) {
 if(page<0)return row<PageCount()?Heading(row):"Back";
 static const char* labels[]={"Player","Bomb","PoC","Toggle Fire","Toggle Focus","Key Config","Screen Mode","Alignment","Focus Hitbox","Quick Info","FPS Counter","Boss Bar","BGM","SFX","Sound Set","Language","Graphic","Player Sprites","Reset Defaults","Back"};
 return labels[static_cast<int>(At(row))];
}
std::string CtrOptions::Value(int row) {
 if(page<0)return "";
 using namespace CtrSettings;
 switch(At(row)) {
  case Item::Lives:return std::to_string(g_Supervisor.cfg.lifeCount+1);
  case Item::Bombs:return std::to_string(g_Supervisor.cfg.bombCount);
  case Item::Collect:return Enabled(Toggle::AutoCollect)?"Always":"Max Power Only";
  case Item::Fire:return Enabled(Toggle::Fire)?"On":"Off";
  case Item::Focus:return Enabled(Toggle::Focus)?"On":"Off";
  case Item::Screen:return Mode()==ScreenMode::TopOnly?"Top Only":"Split";
  case Item::Alignment:return CtrDisplay::AlignmentName();
  case Item::Hitbox:return Enabled(Toggle::Hitbox)?"On":"Off";
  case Item::QuickInfo:return Enabled(Toggle::QuickInfo)?"On":"Off";
  case Item::Fps:return Enabled(Toggle::Fps)?"On":"Off";
  case Item::BossBar:return Enabled(Toggle::BossBar) && CtrBossHealthBar::Available()?"New":"OG";
  case Item::Bgm:return std::to_string(BgmVolume())+"%";
  case Item::Sfx:return std::to_string(SfxVolume())+"%";
  case Item::Sound:return SoundPackName();
  case Item::Language:return English()?"English":"日本語";
  case Item::Graphics:return Retexture()?"Retexture":"OG";
  case Item::Sprites:return NewSprites()?"New":"Old";
  default:return "";
 }
}
bool CtrOptions::Adjustable(int row) {
 if(page<0)return false;
 const auto item=At(row);
 return item!=Item::Keys && item<Item::Reset && !(item==Item::Alignment && CtrSettings::Mode()==CtrSettings::ScreenMode::TopOnly) &&
        !(item==Item::BossBar && !CtrBossHealthBar::Available());
}
int CtrOptions::Update(MainMenu& menu) {
    const int count=RowCount();
    MainMenu::MoveCursor(&menu,count);
    if(menu.stateTimer<CtrMenuTiming::Frames)return 0;
    if(WAS_PRESSED(TH_BUTTON_RETURNMENU)) {menu.cursor=count-1;}
    int direction=WAS_PRESSED_PERIODIC(TH_BUTTON_LEFT)?-1:WAS_PRESSED_PERIODIC(TH_BUTTON_RIGHT)?1:0;
    const bool select=WAS_PRESSED(TH_BUTTON_SELECTMENU);
    if(page<0 && menu.cursor<PageCount()) {
        if(select) {
            page=menu.cursor;menu.cursor=0;menu.stateTimer=0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        }
        return 0;
    }
    if(!direction && select && Adjustable(menu.cursor))direction=1;
    if(direction && Adjustable(menu.cursor)) {
        using namespace CtrSettings;
        switch(At(menu.cursor)) {
            case Item::Lives:g_Supervisor.cfg.lifeCount=(g_Supervisor.cfg.lifeCount+direction+5)%5;break;
            case Item::Bombs:g_Supervisor.cfg.bombCount=(g_Supervisor.cfg.bombCount+direction+4)%4;break;
            case Item::Collect:ChangeToggle(Toggle::AutoCollect);break;
            case Item::Fire:ChangeToggle(Toggle::Fire);break;
            case Item::Focus:ChangeToggle(Toggle::Focus);break;
            case Item::Screen:ChangeMode();break;
            case Item::Alignment:CtrDisplay::Cycle(direction);break;
            case Item::Hitbox:ChangeToggle(Toggle::Hitbox);break;
            case Item::QuickInfo:ChangeToggle(Toggle::QuickInfo);break;
            case Item::Fps:ChangeToggle(Toggle::Fps);break;
            case Item::BossBar:ChangeToggle(Toggle::BossBar);break;
            case Item::Bgm:ChangeVolume(true,direction);break;
            case Item::Sfx:ChangeVolume(false,direction);break;
            case Item::Sound:ChangeContent(Content::Sound,direction);if(!ReloadSounds())return -1;break;
            case Item::Language:ChangeContent(Content::Language,direction);CtrSelectionArt::RefreshLanguage();reloadArtwork=true;break;
            case Item::Graphics:ChangeContent(Content::Graphics,direction);g_Supervisor.cfg.colorMode16bit=0;reloadArtwork=true;if(!CtrPresentation::RefreshMaterial())return -1;break;
            case Item::Sprites:ChangeContent(Content::Sprites,direction);break;
            default:break;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    }
    if(select || WAS_PRESSED(TH_BUTTON_RETURNMENU)) {
        if(page>=0 && At(menu.cursor)==Item::Reset) {
            CtrSettings::Reset();CtrDisplay::Reset();
            if(!CtrPresentation::RefreshMaterial())return -1;
            reloadArtwork=true;
            CtrSelectionArt::RefreshLanguage();
            if(!ReloadSounds())return -1;
            g_Supervisor.cfg.lifeCount=2;g_Supervisor.cfg.bombCount=3;g_Supervisor.cfg.colorMode16bit=0;
            g_Supervisor.cfg.defaultDifficulty=NORMAL;g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        } else if(page>=0 && At(menu.cursor)==Item::Keys) {
            menu.gameState=STATE_KEYCONFIG;menu.stateTimer=0;menu.cursor=0;
            for(auto& vm:menu.vm)vm.pendingInterrupt=5;
            memcpy(menu.controlMapping,&g_ControllerMapping,sizeof(ControllerMapping));
            g_ControllerMapping.upButton=g_ControllerMapping.downButton=-1;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        } else if(menu.cursor==count-1) {
            if(page>=0) {
                menu.cursor=page;page=-1;menu.stateTimer=0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
                return 0;
            }
            if(reloadArtwork) {
                if(MainMenu::LoadTitleAnm(&menu)!=ZUN_SUCCESS)return -1;
                if(g_AnmManager->LoadAnm(ANM_FILE_ASCII,"data/ascii.anm",ANM_OFFSET_ASCII)!=ZUN_SUCCESS ||
                   g_AnmManager->LoadAnm(ANM_FILE_ASCIIS,"data/asciis.anm",ANM_OFFSET_ASCIIS)!=ZUN_SUCCESS)return -1;
                g_AsciiManager.InitializeVms();
                reloadArtwork=false;
            }
            CtrSettings::Save();g_Supervisor.cfg.musicMode=WAV;g_Supervisor.cfg.playSounds=true;
            FileSystem::WriteDataToFile(TH_CONFIG_FILE,&g_Supervisor.cfg,sizeof(g_Supervisor.cfg));
            menu.gameState=STATE_MAIN_MENU;menu.stateTimer=0;menu.cursor=6;
            for(auto& vm:menu.vm)vm.pendingInterrupt=2;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        }
    }
    return 0;
}

#include "RecordsPanel.hpp"
#include "MenuTiming.hpp"
#include "PracticeSelection.hpp"
#include "PracticeMenu.hpp"
#include "SelectionFlow.hpp"
#include "MenuTypeface.hpp"
#include "Presentation.hpp"
#include "DisplaySettings.hpp"
#include "Settings.hpp"
#include "OptionsMenu.hpp"
#include "SelectionLayout.hpp"
#include "SelectionArt.hpp"
#include "CharacterPortraits.hpp"
#include "GameDecorations.hpp"
#include "PanelTransition.hpp"
#include "SelectionReturn.hpp"
#include "GameplayHud.hpp"
#include "Controls.hpp"
#include "MainMenu.hpp"
#include "GameManager.hpp"
#include "AssetPath.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
#include "NativeImage.hpp"
#include "MenuBackdrop.hpp"
#include "MenuCanvas.hpp"
#include "../../../assets/ui-font/ui_labels.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>
namespace {
struct Texture {
    C3D_Tex gpu{};
    Tex3DS_SubTexture sub{};
    bool ready=false;
    void Clear() {if(ready) C3D_TexDelete(&gpu);ready=false;}
    bool Load(const std::string& name) {
        Clear();
        FILE* f=fopen(CtrContent::Path("ui",(name+".bin").c_str()).c_str(),"rb");
        if(!f)return false;
        unsigned h[5]{};
        if(fread(h,sizeof(h),1,f)!=1 || h[0]!=0x44335854 || h[3]<8 || h[4]<8 ||
           h[3]>1024 || h[4]>1024 || h[1]>h[3] || h[2]>h[4]) {fclose(f);return false;}
        if(!C3D_TexInit(&gpu,h[3],h[4],GPU_RGBA8)) {fclose(f);return false;}
        ready=true;
        if(fread(gpu.data,1,gpu.size,f)!=gpu.size) {fclose(f);Clear();return false;}
        fclose(f);C3D_TexSetFilter(&gpu,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(&gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);C3D_TexFlush(&gpu);
        sub={static_cast<u16>(h[1]),static_cast<u16>(h[2]),0,1,float(h[1])/h[3],1-float(h[2])/h[4]};
        return true;
    }
    void Draw(float x=0,float y=0,unsigned alpha=255,unsigned brightness=255) {
        if(ready)CtrNative::DrawImage({&gpu,&sub},x,y,C2D_Color32(brightness,brightness,brightness,alpha));
    }
    void DrawArtwork(unsigned alpha=255,unsigned brightness=255) {
        if(!ready)return;
        Tex3DS_SubTexture inner=sub;
        inner.width=sub.width-2;
        inner.height=sub.height-2;
        inner.left+=1.f/gpu.width;inner.right-=1.f/gpu.width;
        inner.top-=1.f/gpu.height;inner.bottom+=1.f/gpu.height;
        CtrNative::DrawImage({&gpu,&inner},0,0,C2D_Color32(brightness,brightness,brightness,alpha),
            float(sub.width)/inner.width,float(sub.height)/inner.height);
    }
};
Texture previousArtwork;int backgroundFrame=CtrMenuTiming::Frames;
float backgroundMix=1;
unsigned shownBrightness=255,previousBrightness=255;
float contentOpacity=1;
Texture returnArtwork;
struct ListBox {float left=0,top=0,right=0,bottom=0;bool above=false,below=false;float opacity=1;};
std::array<ListBox,2> listBoxes;
unsigned listBoxCount=0;
Texture artwork,mainBottom,creditsBottom,material,font,contentFont;
bool menuAssetsReady=false;
bool EnsureMenuAssets() {
    if(menuAssetsReady)return true;
    menuAssetsReady=CtrMenuTypeface::Initialize() && CtrPractice::Initialize() && mainBottom.Load("Bottom Screen Main Menu") && creditsBottom.Load("Bottom Screen Credits") &&
        CtrCharacterPortraits::Initialize();
    return menuAssetsReady;
}
std::string surfaceName,surfaceVariant,previousSurfaceName,returnName,returnVariant;
// Artwork name and its file variant for a surface path, as LoadSurface keys them.
std::string SurfaceName(const char* path) {
    auto name=CtrAssets::Normalize(path);name=name.substr(name.find_last_of('/')+1);
    return name.substr(0,name.find_last_of('.'));
}
bool UsesBackdrop(const std::string& name) {return name=="result" || name=="music";}
void DrawLowerArtwork(const std::string& name,unsigned alpha,unsigned brightness=255) {
    if(UsesBackdrop(name))CtrMenuBackdrop::Draw(alpha);else mainBottom.Draw(0,0,alpha,brightness);
}
bool MenuSceneChange() {
    const auto side=[](int state) {return state==SUPERVISOR_STATE_RESULTSCREEN || state==SUPERVISOR_STATE_MUSICROOM;};
    const int from=g_Supervisor.wantedState,to=g_Supervisor.curState;
    return (from==SUPERVISOR_STATE_MAINMENU && side(to)) || (side(from) && to==SUPERVISOR_STATE_MAINMENU);
}
enum class Panel {None,Main,Options,Controls,Loading,Selection};
Panel panel=Panel::None;
bool selectionBackdrop=false;
struct Row {std::string label,value;bool selected=false;bool disabled=false;unsigned opacity=255;float slideX=0,slideY=0;};
std::array<Row,11> rows;
int rowCount=0;
std::array<unsigned,8> shades{};
std::array<unsigned,8> overlays{};
unsigned shadeCount=0;
unsigned overlayCount=0,logoOpacity=255;
float headingX=0,headingY=0;
unsigned headingOpacity=255;
bool hasScene=false;
bool openingTitle=false;
int animationFrame=0;
int optionPage=0;
struct PanelSnapshot {
    Panel panel;std::array<Row,11> rows;int rowCount;float headingX,headingY;unsigned headingOpacity;bool opening;int optionPage;
};
CtrPanelTransition<PanelSnapshot> panelTransition;
bool panelBackwards=false,nativeMainEntrance=true;
PanelSnapshot Snapshot(){return {panel,rows,rowCount,headingX,headingY,headingOpacity,openingTitle,optionPage};}
void Restore(const PanelSnapshot& value){panel=value.panel;rows=value.rows;rowCount=value.rowCount;headingX=value.headingX;headingY=value.headingY;headingOpacity=value.headingOpacity;openingTitle=value.opening;optionPage=value.optionPage;}

unsigned RGBA(unsigned argb) {return (argb&0xff00ff00)|((argb&255)<<16)|((argb>>16)&255);}
void Shade(float w,float h) {
    for(unsigned i=0;i<shadeCount;i++) C2D_DrawRectSolid(0,0,0,w,h,RGBA(shades[i]));
}
const CtrUiAssets::Label* Find(const std::string& text) {
    const CtrUiAssets::Label* fallback=nullptr;
    for(const auto& entry:CtrUiAssets::labels)if(text==entry.text) {
        if(!strcmp(entry.style,"option")) {if(panel==Panel::Options)return &entry;}
        else if(!fallback)fallback=&entry;
    }
    return fallback;
}
void Label(const std::string& text,float x,float y,bool selected=false,bool disabled=false,bool centered=false,unsigned opacity=255,float sx=1,float sy=1) {
    const auto* l=Find(text);if(!l || !font.ready || !opacity)return;
    auto& sheet=!strcmp(l->style,"content")?contentFont:font;
    if(!sheet.ready)return;
    Tex3DS_SubTexture sub{static_cast<u16>(l->w),static_cast<u16>(l->h),
        float(l->x)/sheet.gpu.width,1-float(l->y)/sheet.gpu.height,float(l->x+l->w)/sheet.gpu.width,1-float(l->y+l->h)/sheet.gpu.height};
    const unsigned color=disabled?C2D_Color32(125,125,125,opacity):
        selected?((panel==Panel::Main || panel==Panel::Loading)?C2D_Color32(255,0,0,opacity):C2D_Color32(255,255,128,opacity)):C2D_Color32(255,255,255,opacity);
    if(selected && !disabled)CtrNative::DrawTextGradient({&sheet.gpu,&sub},centered?x-l->w*sx/2.f:x-5*sx,y-5*sy,color,l->faceTop,l->faceBottom,sx,sy);
    else CtrNative::DrawImage({&sheet.gpu,&sub},centered?x-l->w*sx/2.f:x-5*sx,y-5*sy,color,sx,sy);
}
}
bool CtrPresentation::Initialize() {
    return EnsureMenuAssets() && CtrSelectionArt::Initialize() && CtrHud::Initialize() &&
        material.Load("gameplay-material") && font.Load("ui-font") && contentFont.Load("content-font");
}
bool CtrPresentation::RefreshMaterial() {
    if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    return material.Load("gameplay-material");
}
void CtrPresentation::ReleaseMenuAssets() {
    CtrRecordsPanel::Release();
    if(!menuAssetsReady)return;
    C3D_FrameSync();
    for(Texture* t:{&previousArtwork,&artwork,&returnArtwork,&mainBottom,&creditsBottom})t->Clear();
    CtrCharacterPortraits::Shutdown();CtrPractice::Shutdown();CtrMenuTypeface::Shutdown();
    surfaceName.clear();surfaceVariant.clear();menuAssetsReady=false;
}
void CtrPresentation::ReleaseMenuText() {
    if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    CtrPractice::Shutdown();CtrMenuTypeface::Shutdown();menuAssetsReady=false;
}
void CtrPresentation::Shutdown() {
    CtrDecorations::Shutdown();CtrSelectionArt::Shutdown();CtrCharacterPortraits::Shutdown();CtrPractice::Shutdown();CtrMenuTypeface::Shutdown();CtrHud::Shutdown();
    for(Texture* t:{&previousArtwork,&artwork,&returnArtwork,&mainBottom,&creditsBottom,&material,&font,&contentFont})t->Clear();
    menuAssetsReady=false;surfaceName.clear();surfaceVariant.clear();
}
void CtrPresentation::BeginScene() {
    CtrSelection::BeginScene();CtrRecordsPanel::Clear();panel=Panel::None;shadeCount=overlayCount=0;hasScene=false;logoOpacity=255;
    contentOpacity=1;listBoxCount=0;
}
void CtrPresentation::MarkScene() {hasScene=true;}
bool CtrPresentation::HasScene() {return hasScene;}
bool CtrPresentation::ReplacesSurface(int slot) {return slot==0 && artwork.ready;}
void CtrPresentation::LoadSurface(int slot,const char* path) {
    if(slot!=0)return;
    if(!EnsureMenuAssets())return;
    const auto name=SurfaceName(path);
    const auto variant=CtrContent::Path("ui",(name+".bin").c_str());
    if(returnArtwork.ready && name==returnName && variant==returnVariant) {
        previousArtwork.Clear();previousSurfaceName.clear();artwork.Clear();
        artwork=returnArtwork;returnArtwork={};
        surfaceName=returnName;surfaceVariant=returnVariant;
        returnName.clear();returnVariant.clear();
        backgroundFrame=CtrMenuTiming::Frames;backgroundMix=1;
        if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->DiscardMenuLayer();
        return;
    }
    if(artwork.ready && name==surfaceName && variant==surfaceVariant)return;
    if(artwork.ready || !previousArtwork.ready) {
        previousArtwork.Clear();previousSurfaceName=surfaceName;
        if(panel==Panel::Loading)artwork.Clear();else previousArtwork=artwork;
        previousBrightness=shownBrightness;
    }
    artwork={};backgroundFrame=previousArtwork.ready?0:CtrMenuTiming::Frames;
    surfaceName.clear();surfaceVariant.clear();
    if(artwork.Load(name)) {surfaceName=name;surfaceVariant=variant;}
}
void CtrPresentation::ReleaseSurface(int slot) {
    if(slot!=0 || (!artwork.ready && !previousArtwork.ready))return;
    if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    previousArtwork.Clear();previousSurfaceName.clear();
    backgroundFrame=CtrMenuTiming::Frames;backgroundMix=1;
    if(MenuSceneChange() && g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->DiscardMenuLayer();
    if(MenuSceneChange() && returnArtwork.ready) {
        artwork.Clear();artwork=returnArtwork;returnArtwork={};
        surfaceName=returnName;surfaceVariant=returnVariant;
        return;
    }
    if(MenuSceneChange() && artwork.ready) {
        previousArtwork=artwork;artwork={};previousSurfaceName=surfaceName;
        previousBrightness=shownBrightness;
    } else artwork.Clear();
    surfaceName.clear();surfaceVariant.clear();
}
void CtrPresentation::PrepareReturn(const char* path) {
    if(returnArtwork.ready || !EnsureMenuAssets())return;
    returnName=SurfaceName(path);
    returnVariant=CtrContent::Path("ui",(returnName+".bin").c_str());
    if(!returnArtwork.Load(returnName))returnName.clear();
}
void CtrPresentation::CaptureShade(float left,float top,float right,float bottom,unsigned color,bool backgroundOnly) {
    if(left>0 || top>0 || right<640 || bottom<480)return;
    if(backgroundOnly) {
        if(shadeCount<shades.size())shades[shadeCount++]=color;
        if(panel==Panel::Loading && (color&0xffffff)==0)logoOpacity=255-(color>>24);
    } else if(overlayCount<overlays.size())overlays[overlayCount++]=color;
}
bool CtrPresentation::ReplacesMenu(const MainMenu& menu,int vmIndex) {
    if(vmIndex>=27 && vmIndex<=33) return !(menu.gameState==STATE_PRE_INPUT ||
        menu.gameState==STATE_MAIN_MENU || menu.gameState==STATE_OPTIONS ||
        menu.gameState==STATE_KEYCONFIG || menu.gameState==STATE_DIFFICULTY_LOAD ||
        menu.gameState==STATE_CHARACTER_LOAD);
    if(vmIndex>=80 && vmIndex<=95)return true;
    return vmIndex<80;
}
void CtrPresentation::CaptureMenu(const MainMenu& menu) {
    MarkScene();
    openingTitle=menu.gameState==STATE_PRE_INPUT;animationFrame=menu.stateTimer;
    selectionBackdrop=false;
    auto add=[&](int i,const char* label,const std::string& value="") {
        rows[i]={label,value,i==menu.cursor,false};
    };
    auto styleRow=[&](int i,float selectionOffset) {
        auto& row=rows[i];
        row.opacity=row.selected?255:128;
        row.slideX=row.slideY=row.selected?selectionOffset:0;
    };
    auto captureMain=[&]() {
        rowCount=7;
        const char* labels[]={"Start","Extra Start","Practice Start","Replay","Score","Music Room","Option"};
        for(int i=0;i<7;i++) {
            add(i,labels[i]);
            const auto& vm=menu.vm[i];
            rows[i].opacity=vm.flags.isVisible?(vm.color>>24):0;
            const float restingX=CtrUiAssets::menuRestX[i];
            rows[i].slideX=(vm.pos.x-restingX)/(640-restingX)*302 + vm.posOffset.x*0.5f;
            rows[i].slideY=vm.posOffset.y*0.5f;
        }
        rows[1].disabled=!(g_GameManager.HasReachedMaxClears(0,0) || g_GameManager.HasReachedMaxClears(0,1) ||
            g_GameManager.HasReachedMaxClears(1,0) || g_GameManager.HasReachedMaxClears(1,1));
    };
    if(menu.gameState==STATE_CHARACTER_LOAD || menu.gameState==STATE_STARTUP) {
        panel=Panel::Selection;rowCount=0;
    } else if(menu.gameState==STATE_DIFFICULTY_LOAD || menu.gameState==STATE_REPLAY_LOAD ||
       menu.gameState==STATE_SCORE || menu.gameState==STATE_MUSIC_ROOM) {
        panel=Panel::Loading;captureMain();
        const float opacity=1-CtrMenuTiming::Progress(animationFrame);
        for(auto& row:rows)row.opacity=unsigned(row.opacity*opacity);
    } else if(IsSelection(menu)) {
        panel=Panel::Selection;rowCount=0;selectionBackdrop=true;
    } else if(menu.gameState==STATE_MAIN_MENU || menu.gameState==STATE_PRE_INPUT || menu.gameState==STATE_QUIT) {
        panel=Panel::Main;captureMain();
    } else if(menu.gameState==STATE_OPTIONS) {
        panel=Panel::Options;rowCount=CtrOptions::RowCount();
        optionPage=CtrOptions::Page();
        for(int i=0;i<rowCount;i++) {
            add(i,CtrOptions::Label(i),CtrOptions::Value(i));
            rows[i].disabled=!CtrOptions::Adjustable(i) && !rows[i].value.empty();
        }
        for(int i=0;i<rowCount;i++)styleRow(i,optionPage<0?0:-3);
        headingX=headingY=0;headingOpacity=255;
    } else if(menu.gameState==STATE_KEYCONFIG) {
        panel=Panel::Controls;rowCount=11;
        const char* labels[]={"Shot","Bomb","Focus","Pause","Up","Down","Left","Right","Skip"};
        for(int i=0;i<9;i++)add(i,labels[i],CtrControls::Capturing() && i==menu.cursor?
            "Press Button":CtrControls::Name(menu.controlMapping[i]));
        add(9,"Reset Defaults");add(10,"Back");
        for(int i=0;i<rowCount;i++)styleRow(i,-2);
        headingX=headingY=0;headingOpacity=255;
    }
    if(panel==Panel::Main || panel==Panel::Options || panel==Panel::Controls) {
        const int next=int(panel)*10+(panel==Panel::Options?optionPage:0);
        if(next!=panelTransition.key)panelBackwards=CtrSelectionReturn::From(STATE_DIFFICULTY_SELECT) ||
            (panelTransition.key>=0 && (int(panel)<int(panelTransition.current.panel) ||
             (panel==Panel::Options && panelTransition.current.panel==Panel::Options && optionPage<panelTransition.current.optionPage)));
        if(panelTransition.key<0 && CtrSelectionReturn::From(STATE_DIFFICULTY_SELECT)) {
            panelTransition.key=999;panelTransition.current=PanelSnapshot{};
        }
        if(panel==Panel::Main) {
            if(next!=panelTransition.key)nativeMainEntrance=panelTransition.key<0;
            if(!nativeMainEntrance)for(int i=0;i<rowCount;i++)styleRow(i,-2);
        }
        panelTransition.Capture(next,Snapshot());
    }

}
void CtrPresentation::DrawBackground(C3D_RenderTarget* top,C3D_RenderTarget* bottom,bool gameplay) {
    const bool replayMenu=g_Supervisor.wantedState==SUPERVISOR_STATE_MAINMENU &&
        (g_MainMenu.gameState==STATE_REPLAY_ANIM || g_MainMenu.gameState==STATE_REPLAY_SELECT ||
         g_MainMenu.gameState==STATE_REPLAY_UNLOAD);
    C2D_SceneBegin(top);
    if(gameplay)material.Draw();else if(artwork.ready) {
        artwork.DrawArtwork();
        if(backgroundFrame<CtrMenuTiming::Frames) {
            backgroundMix=CtrMenuTiming::Progress(backgroundFrame++);
            previousArtwork.DrawArtwork(unsigned(255*(1-backgroundMix)),previousBrightness);
        } else {previousArtwork.Clear();backgroundMix=1;}
        Shade(400,240);
        shownBrightness=CtrPresentation::SceneOpacity();
    }
    else material.Draw();
    C2D_SceneBegin(bottom);
    if(gameplay)material.Draw(-40,-240);
    else if(surfaceName=="th06logo")C2D_DrawRectSolid(0,0,0,320,240,0xffffffff);
    else if(panel==Panel::Selection) {
        C2D_DrawRectSolid(0,0,0,320,240,0xff000000);
        if(selectionBackdrop)CtrMenuBackdrop::Draw(CtrPresentation::SceneOpacity());
    }
    else if(surfaceName.starts_with("end") || surfaceName=="staff00")creditsBottom.Draw();
    else if(replayMenu || UsesBackdrop(surfaceName)) {
        CtrMenuBackdrop::Draw();
        if(backgroundMix<1 && !replayMenu && !previousSurfaceName.empty())
            DrawLowerArtwork(previousSurfaceName,unsigned(255*(1-backgroundMix)),previousBrightness);
    }
    else {
        mainBottom.Draw();
        if(backgroundMix<1 && UsesBackdrop(previousSurfaceName))CtrMenuBackdrop::Draw(unsigned(255*(1-backgroundMix)));
        if(CtrSelectionReturn::From(STATE_DIFFICULTY_SELECT))
            C2D_DrawRectSolid(0,0,0,320,240,unsigned(255*(1-CtrSelectionReturn::Progress()))<<24);
        Shade(320,240);
    }
}
void DrawPanelInternal(C3D_RenderTarget* bottom) {
    if(panel==Panel::None)return;
    if(panel==Panel::Selection) {C2D_SceneBegin(bottom);CtrSelection::DrawTouchPanel();return;}
    C2D_SceneBegin(bottom);
    C2D_SetTintMode(C2D_TintMult);
    if(panel==Panel::Main || panel==Panel::Loading) {
        if(openingTitle) {
            const unsigned pulse=160+unsigned(95*(.5f+.5f*std::sin(animationFrame*.06f)));
            Label("Press any button",160,112,false,false,true,pulse);
            C2D_SetTintMode(C2D_TintSolid);return;
        }
        for(int i=0;i<rowCount;i++) Label(rows[i].label,24+rows[i].slideX,14+i*26+rows[i].slideY,rows[i].selected,rows[i].disabled,false,rows[i].opacity);
        const auto* version=Find("3DS Version 1.0.4");
        unsigned opacity=0;
        for(int i=0;i<rowCount;i++)opacity=std::max(opacity,rows[i].opacity);
        if(version)Label(version->text,315-version->w,216,false,false,false,opacity);
    } else {
        Label(panel==Panel::Controls?"Controls":CtrOptions::Heading(optionPage),160+headingX,6+headingY,false,false,true,headingOpacity);
        const bool categories=panel==Panel::Options && optionPage<0;
        const float y0=panel==Panel::Controls?30:categories?CtrOptions::CategoriesTop:CtrOptions::RowsTop;
        const float spacing=panel==Panel::Controls?17:categories?CtrOptions::CategorySpacing:CtrOptions::RowSpacing;
        for(int i=0;i<rowCount;i++) {
            const auto& r=rows[i];const float y=y0+i*spacing+r.slideY;
            Label(r.label,22+r.slideX,y,r.selected,r.disabled,false,r.opacity);
            if(!r.value.empty()) {const auto* value=Find(r.value);Label(r.value,(panel==Panel::Options && value?315-value->w:196)+r.slideX,y,r.selected,r.disabled,false,r.opacity);}
            if(r.selected && panel==Panel::Controls) {
                const unsigned pulse=192+unsigned(63*(0.5f+0.5f*std::sin(animationFrame*.15f)));
                Label(">",7+r.slideX,y,true,false,false,r.opacity*pulse/255);
            }
        }
        if(panel==Panel::Controls)Label("A: Change   B: Save   SELECT: Unbind",160+headingX,222+headingY,true,false,true,headingOpacity);
    }
    C2D_SetTintMode(C2D_TintSolid);
}

void CtrPresentation::DrawMenu(C3D_RenderTarget* bottom) {
    if(panel!=Panel::Main && panel!=Panel::Options && panel!=Panel::Controls) {
        panelTransition.Reset();DrawPanelInternal(bottom);return;
    }
    const auto saved=Snapshot();const float p=panelTransition.Progress();
    const float direction=panelBackwards?1.f:-1.f;
    if(p<1) {
        auto outgoing=panelTransition.previous;
        for(auto& row:outgoing.rows){row.slideX+=direction*320*p;row.opacity=unsigned(row.opacity*(1-p));}
        outgoing.headingX+=direction*320*p;outgoing.headingOpacity=unsigned(outgoing.headingOpacity*(1-p));
        Restore(outgoing);DrawPanelInternal(bottom);
        auto incoming=saved;
        for(auto& row:incoming.rows){row.slideX+=-direction*320*(1-p);row.opacity=unsigned(row.opacity*p);}
        incoming.headingX+=-direction*320*(1-p);incoming.headingOpacity=unsigned(incoming.headingOpacity*p);
        Restore(incoming);
    }
    DrawPanelInternal(bottom);Restore(saved);
}

bool CtrPresentation::IsSelection(const MainMenu& menu) {
    return menu.gameState==STATE_DIFFICULTY_SELECT || menu.gameState==STATE_CHARACTER_SELECT ||
        menu.gameState==STATE_SHOT_SELECT || menu.gameState==STATE_PRACTICE_LVL_SELECT ||
        menu.gameState==STATE_PRACTICE_TYPE || menu.gameState==STATE_PRACTICE_SETUP;
}
void CtrPresentation::DrawOverlays(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    for(unsigned i=0;i<overlayCount;i++) {
        C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,400,240,RGBA(overlays[i]));
        C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,320,240,RGBA(overlays[i]));
    }
}
void CtrPresentation::DrawText(const char* text,float x,float y,bool selected,unsigned opacity,bool centered,float sx,float sy) {
    C2D_SetTintMode(C2D_TintMult);Label(text,x,y,selected,false,centered,opacity,sx,sy);C2D_SetTintMode(C2D_TintSolid);
}
unsigned CtrPresentation::LogoOpacity() {return logoOpacity;}
unsigned CtrPresentation::SceneOpacity() {
    unsigned opacity=255;
    for(unsigned i=0;i<shadeCount;i++)if((shades[i]&0xffffff)==0)
        opacity=opacity*(255-(shades[i]>>24))/255;
    return opacity;
}
void CtrPresentation::TouchMenu(MainMenu& menu,int x,int y) {
    if(CtrSelectionFlow::Touch(x,y))return;
    if(CtrPracticeMenu::Touch(menu,x,y))return;
    auto press=[&](unsigned button) {g_CurFrameInput|=button;g_LastFrameInput&=~button;};
    if(menu.gameState==STATE_PRE_INPUT) {press(TH_BUTTON_ENTER);return;}
    if(menu.stateTimer<CtrMenuTiming::Frames)return;
    if(panel==Panel::Main || panel==Panel::Options || panel==Panel::Controls) {
        const bool categories=panel==Panel::Options && optionPage<0;
        const float start=panel==Panel::Main?14:panel==Panel::Options?(categories?CtrOptions::CategoriesTop:CtrOptions::RowsTop):30;
        const float spacing=panel==Panel::Main?26:panel==Panel::Options?(categories?CtrOptions::CategorySpacing:CtrOptions::RowSpacing):17;
        for(int i=0;i<rowCount;i++) {
            const auto& r=rows[i];const float rowY=start+i*spacing+r.slideY;
            if(r.disabled || !r.opacity || x<r.slideX || x>=320+r.slideX || y<rowY-4 || y>=rowY+spacing-4)continue;
            menu.cursor=i;
            if(panel==Panel::Controls)CtrControls::SelectByTouch();
            else press(panel==Panel::Options && CtrOptions::Adjustable(i)?TH_BUTTON_RIGHT:TH_BUTTON_SELECTMENU);
            return;
        }
    }
    CtrSelection::Touch(menu,x,y);
}

void CtrPresentation::SetContentOpacity(float opacity) {contentOpacity=std::clamp(opacity,0.f,1.f);}
void CtrPresentation::ShowListBox(float left,float top,float right,float bottom,bool moreAbove,bool moreBelow,float opacity) {
    if(listBoxCount<listBoxes.size())listBoxes[listBoxCount++]={left,top,right,bottom,moreAbove,moreBelow,std::clamp(opacity,0.f,1.f)};
}
void CtrPresentation::DrawListBox(C3D_RenderTarget* top) {
    if(!listBoxCount)return;
    C2D_SceneBegin(top);
    for(unsigned i=0;i<listBoxCount;i++) {
        const auto& box=listBoxes[i];
        const float origin=CtrMenuCanvas::ScreenLeft();
        const float left=origin+box.left/2,right=origin+box.right/2,y0=box.top/2,y1=box.bottom/2;
        C2D_DrawRectSolid(left,y0,0,right-left,y1-y0,C2D_Color32(0,0,0,unsigned(150*box.opacity)));
        const unsigned color=C2D_Color32(255,255,255,unsigned(200*box.opacity));
        const float x=right-8,half=4,height=4;
        if(box.above)C2D_DrawTriangle(x-half,y0+3+height,color,x+half,y0+3+height,color,x,y0+3,color,0);
        if(box.below)C2D_DrawTriangle(x-half,y1-3-height,color,x+half,y1-3-height,color,x,y1-3,color,0);
    }
}
void CtrPresentation::DrawContentFade(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!artwork.ready)return;
    if(returnArtwork.ready) {
        const unsigned cover=unsigned(255*(1-contentOpacity));
        C2D_SceneBegin(top);returnArtwork.DrawArtwork(cover);
        C2D_SceneBegin(bottom);mainBottom.Draw(0,0,cover);
        return;
    }
    const bool crossfading=backgroundMix<1 && previousArtwork.ready;
    const float shown=std::min(contentOpacity,crossfading?backgroundMix:1.f);
    if(shown>=1)return;
    const float cover=1-shown,mix=crossfading?backgroundMix:1;
    const float outgoing=cover*(1-mix),incoming=outgoing<1?cover*mix/(1-outgoing):0;
    C2D_SceneBegin(top);
    artwork.DrawArtwork(unsigned(255*incoming));
    if(crossfading)previousArtwork.DrawArtwork(unsigned(255*outgoing),previousBrightness);
    C2D_SceneBegin(bottom);
    DrawLowerArtwork(surfaceName,unsigned(255*incoming));
    if(crossfading && !previousSurfaceName.empty())DrawLowerArtwork(previousSurfaceName,unsigned(255*outgoing),previousBrightness);
}

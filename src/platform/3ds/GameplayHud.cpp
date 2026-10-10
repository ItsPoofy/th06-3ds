#include "GameplayHud.hpp"
#include "DisplaySettings.hpp"
#include "GameplayLayout.hpp"
#include "GameManager.hpp"
#include "SelectionArt.hpp"
#include "NativeImage.hpp"
#include "GameplayLogo.hpp"
#include "TextSizes.hpp"
#include "Settings.hpp"
#include "AnmManager.hpp"
#include "CtrRenderer.hpp"
#include <cstring>
#include <algorithm>
#include <cstdio>
namespace {
char fpsText[24]{};
// Crop the live original HUD, preserving its font, colored stars and power bar.
void RowPart(C3D_Tex& scene,int sourceY,int sourceX,int width,float x,float y,float scale) {
    // The engine's top-left origin maps to y=480 in the render target.
    // Framebuffer UVs run upwards; using sourceY directly samples the logo
    // at the bottom of the original sidebar instead of its live HUD rows.
    Tex3DS_SubTexture sub{u16(width),20,float(sourceX)/1024,(480.f-sourceY)/512,
                         float(sourceX+width)/1024,(480.f-sourceY-20.f)/512};
    C2D_DrawImageAt({&scene,&sub},x,y,0,nullptr,scale,scale);
}
void Row(C3D_Tex& scene,int sourceY,float x,float y,float scale=.70f) {
    RowPart(scene,sourceY,432,192,x,y,scale);
}
void Number(unsigned value,float x,float y,bool score=false) {
    char text[16];snprintf(text,sizeof(text),score?"%09u":"%u",value);
    for(const char* digit=text;*digit;digit++,x+=14) {
        char label[24];snprintf(label,sizeof(label),"hud-digit%c",*digit);
        CtrSelectionArt::Draw(label,x,y);
    }
}
void Stat(const char* label,unsigned value,float x,float y,bool score=false) {
    CtrSelectionArt::Draw(label,x,y);
    Number(value,x,y+20,score);
}
void Stars(const char* label,const char* star,int count,float x,float y) {
    CtrSelectionArt::Draw(label,x,y);
    for(int i=0;i<std::clamp(count,0,8);i++)CtrSelectionArt::Draw(star,x+i*16,y+20);
}
void Indicators(C3D_RenderTarget* bottom) {
    C2D_SceneBegin(bottom);char stage[24];snprintf(stage,sizeof(stage),"Stage %d",std::clamp(int(g_GameManager.currentStage),1,7));
    const int d=g_GameManager.difficulty;
    const float y=CtrLayout::Single()?194.f:CtrDisplay::GetAlignment()==CtrDisplay::Alignment::Top?140.f:CtrDisplay::GetAlignment()==CtrDisplay::Alignment::Middle?220.f:4.f;
    // Plain color-coded status line; the material remains visible underneath.
    CtrSelectionArt::Draw(CtrSelectionArt::DifficultyName(d),7,y,CtrSelectionArt::DifficultyColor(d),false,CtrTextSizes::StatusDifficulty);
    CtrSelectionArt::Draw(stage,100,y,CtrSelectionArt::DifficultyColor(d),false,CtrTextSizes::StatusStage);
}
void Fps(float right,float y) {
    // Draw transparent native glyphs; copying the scene also copied its sky
    // or sidebar backdrop into an opaque rectangle behind the counter.
    if(!g_AnmManager || !CtrSettings::Enabled(CtrSettings::Toggle::Fps))return;
    auto* renderer=static_cast<CtrRenderer*>(g_GfxBackend);
    float x=right-14*CtrTextSizes::Utility*std::strlen(fpsText);
    for(const unsigned char* ch=reinterpret_cast<const unsigned char*>(fpsText);*ch;ch++,x+=14*CtrTextSizes::Utility) {
        if(*ch<0x20 || *ch>0x7e)continue;
        const auto& sprite=g_AnmManager->sprites[*ch-0x15];
        if(sprite.sourceFileIndex<0 || sprite.sourceFileIndex>=264)return;
        auto* texture=renderer->TextureImage(g_AnmManager->textures[sprite.sourceFileIndex].handle);
        if(!texture)continue;
        Tex3DS_SubTexture sub{u16(sprite.widthPx),u16(sprite.heightPx),sprite.uvStart.x,1-sprite.uvStart.y,sprite.uvEnd.x,1-sprite.uvEnd.y};
        CtrNative::DrawImage({texture,&sub},x,y,0xffffffff,CtrTextSizes::Utility,CtrTextSizes::Utility);
    }
}
}
void CtrHud::DrawMenuFps(C3D_RenderTarget* top) {C2D_SceneBegin(top);Fps(396,226);}
void CtrHud::SetFps(const char* text) {std::snprintf(fpsText,sizeof(fpsText),"%s",text);}
bool CtrHud::Initialize() {return CtrGameplayLogo::Initialize();}
void CtrHud::Shutdown() {CtrGameplayLogo::Shutdown();}
void CtrHud::Draw(C3D_Tex& scene,C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(CtrLayout::Single()) {
        C2D_SceneBegin(bottom);
        // Keep the decorative EoSD mark large, centered and translucent, then
        // spread the live original HUD rows across the available touch screen.
        CtrGameplayLogo::Draw();
        Stat("hud-score",g_GameManager.guiScore,8,8,true);
        Stat("hud-high",g_GameManager.highScore,174,8,true);
        Stars("hud-player","hud-life-star",g_GameManager.livesRemaining,8,64);
        Stars("hud-bomb","hud-bomb-star",g_GameManager.bombsRemaining,174,64);
        CtrSelectionArt::Draw("hud-power",8,120);
        const unsigned power=std::min(128u,unsigned(g_GameManager.currentPower));
        if(power)C2D_DrawRectSolid(8,140,0,power,16,C2D_Color32(210,210,235,180));
        if(power==128)CtrSelectionArt::Draw("hud-max",8,140);else Number(power,8,140);
        Stat("hud-graze",g_GameManager.grazeInStage,174,120);
        Stat("hud-points",g_GameManager.pointItemsCollectedInStage,8,176);
        Fps(316,226);return;
    }
    if(CtrDisplay::GetAlignment()==CtrDisplay::Alignment::Middle) {
        C2D_SceneBegin(top);
        Row(scene,54,48,8,.78f);Row(scene,78,218,8,.78f);
        Fps(396,32);
        C2D_SceneBegin(bottom);
        Row(scene,118,10,188);Row(scene,142,10,204);
        Row(scene,182,172,188);Row(scene,202,172,204);Row(scene,222,172,220);
    } else {
        const bool atTop=CtrDisplay::GetAlignment()==CtrDisplay::Alignment::Top;
        C2D_SceneBegin(atTop?bottom:top);
        const float base=atTop?140.f:10.f;
        const float left=atTop?10.f:50.f,right=atTop?172.f:212.f;
        Row(scene,118,left,base+20);Row(scene,142,left,base+50);
        Row(scene,54,right,base);Row(scene,78,right,base+18);
        Row(scene,182,right,base+36);Row(scene,202,right,base+54);Row(scene,222,right,base+72);
        Fps(atTop?316.f:396.f,atTop?226.f:96.f);
    }
    Indicators(bottom);
}

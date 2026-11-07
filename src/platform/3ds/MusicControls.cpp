#include "MusicControls.hpp"
#include "AsciiManager.hpp"
#include "NdspAudio.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
// Positions are menu-layer coordinates (640x480), shown at half size on the
// lower screen. ASCII glyphs advance 14 pixels at scale 1.
constexpr float GlyphAdvance=14;
constexpr float LayerScale=2;

constexpr float BarScale=1.25f,BarY=344;
constexpr int TimeChars=6,BarCells=20;
constexpr float BarLeft=(640-(2*TimeChars+BarCells)*GlyphAdvance*BarScale)/2;
constexpr float BarCellsLeft=BarLeft+TimeChars*GlyphAdvance*BarScale;
constexpr float BarCellsRight=BarCellsLeft+BarCells*GlyphAdvance*BarScale;

constexpr float ButtonScale=1.5f,ButtonY=412,ButtonTouchTop=390;
constexpr float ButtonCenters[]={120,320,520};
constexpr CtrMusicControls::Action Buttons[]={CtrMusicControls::Action::Previous,
    CtrMusicControls::Action::StartStop,CtrMusicControls::Action::Next};

constexpr unsigned NormalColor=0xe0c0c0c0,ActiveColor=0xffffffff,PressedColor=0xffff6060;
constexpr int HighlightFrames=8;
CtrMusicControls::Action highlighted=CtrMusicControls::Action::None;
int highlightFrames=0;

void Text(const char* text,float x,float y,float scale,unsigned color) {
    g_AsciiManager.scale=ZunVec2(scale,scale);
    g_AsciiManager.color=color;
    ZunVec3 position(x,y,0);
    g_AsciiManager.AddString(&position,text);
}

void DrawTimes(bool playing,float seconds,float duration) {
    char elapsed[TimeChars]="--:--",total[TimeChars]="--:--";
    if(playing) {
        std::snprintf(elapsed,sizeof(elapsed),"%02d:%02d",int(seconds)/60,int(seconds)%60);
        std::snprintf(total,sizeof(total),"%02d:%02d",int(duration)/60,int(duration)%60);
    }
    Text(elapsed,BarLeft,BarY,BarScale,NormalColor);
    Text(total,BarCellsRight+GlyphAdvance*BarScale,BarY,BarScale,NormalColor);
}
}

void CtrMusicControls::Draw() {
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    const auto savedGui=g_AsciiManager.isGui;const auto savedLower=g_AsciiManager.lowerScreen;
    g_AsciiManager.isGui=false;g_AsciiManager.lowerScreen=true;

    float seconds=0,duration=0;
    const bool playing=CtrAudio::MusicPosition(seconds,duration);
    DrawTimes(playing,seconds,duration);
    const char* labels[]={"<< PREV",playing?"STOP":"START","NEXT >>"};
    for(int i=0;i<3;i++) {
        const float width=(GlyphAdvance*std::strlen(labels[i])+2)*ButtonScale;
        const bool pressed=highlightFrames>0 && highlighted==Buttons[i];
        const unsigned color=pressed?PressedColor:Buttons[i]==Action::StartStop?ActiveColor:NormalColor;
        Text(labels[i],ButtonCenters[i]-width/2,ButtonY,ButtonScale,color);
    }
    if(highlightFrames>0)highlightFrames--;

    g_AsciiManager.scale=savedScale;g_AsciiManager.color=savedColor;
    g_AsciiManager.isGui=savedGui;g_AsciiManager.lowerScreen=savedLower;
}

CtrMusicControls::Action CtrMusicControls::Hit(int x,int y) {
    const float layerX=x*LayerScale,layerY=y*LayerScale;
    if(layerY<ButtonTouchTop)return Action::None;
    // Each button owns a third of the lower screen's width.
    highlighted=Buttons[std::clamp(int(layerX*3/640),0,2)];
    highlightFrames=HighlightFrames;
    return highlighted;
}

#include "RecordsLayout.hpp"
#include "RecordsPanel.hpp"
#include "Presentation.hpp"
#include "CtrRenderer.hpp"
#include "ResultScreen.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Controller.hpp"
#include "SoundPlayer.hpp"
#include "TextHelper.hpp"
#include "i18n.hpp"
#include "ContentAssets.hpp"
#include "thirdparty/sjis_converter.h"
#include "utils.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <cstdlib>
#include <string>

bool isUTF8Encoded(const char* string);

namespace {
const int CardCount=std::size(g_GameManager.catk);

constexpr int FontSize=15;
constexpr float TextRowWidth=240;
constexpr int RowSlots=5,DetailSlot=14;
int prepared[RowSlots];
int preparedDetail=-1;
int offset=0;

const char* Name(int index) {
    const auto& card=g_GameManager.catk[index];
    return card.numAttempts?card.name:TH_UNKNOWN_SPELLCARD;
}
unsigned Color(int index) {
    const auto& card=g_GameManager.catk[index];
    return card.numAttempts==0?0x80c0c0ffu:card.numSuccess==0?0xffc0a0a0u:0xfff0f0ffu;
}

int FitSize(const char* text) {
    int size=FontSize;
    while(size>8 && TextHelper::Measure(text,size)>TextRowWidth)size--;
    return size;
}

void Prepare(ResultScreen& screen,int index) {
    const int slot=index%RowSlots;
    if(prepared[slot]==index)return;
    auto& vm=screen.unk_28a0[slot];
    vm.fontWidth=vm.fontHeight=FontSize;
    TextHelper::Wrapped(&vm,Name(index),COLOR_RGB(COLOR_WHITE),COLOR_RGB(COLOR_BLACK),286);
    prepared[slot]=index;
}

void DrawPart(AnmVm vm,float start,float width,float x,float y,float scaleX,float scaleY,unsigned color) {
    if(!vm.sprite)return;
    auto sprite=*vm.sprite;
    sprite.uvStart.x+=start/sprite.textureWidth;
    sprite.widthPx=width;
    sprite.heightPx=vm.ctrTextHeight;
    sprite.uvEnd.x=sprite.uvStart.x+sprite.widthPx/sprite.textureWidth;
    sprite.uvEnd.y=(sprite.startPixelInclusive.y+sprite.heightPx-.5f)/sprite.textureHeight;
    vm.sprite=&sprite;vm.flags.anchor=AnmVmAnchor_TopLeft;
    vm.scaleX=scaleX;vm.scaleY=scaleY;
    vm.posOffset=ZunVec3(0,0,0);vm.pos=ZunVec3(x,y,0);vm.color=color;
    g_AnmManager->DrawNoRotation(&vm);
    // AnmManager caches UVs by sprite address, and this copy lives on the stack.
    g_AnmManager->SetCurrentSprite(nullptr);
}

float TextHeightScale(const AnmVm& vm,float scale) {return scale*FontSize/(vm.fontHeight>0?vm.fontHeight:FontSize);}
float TextWidthScale(const AnmVm& vm,float scale,float maxWidth) {
    return std::min(TextHeightScale(vm,scale),maxWidth/std::max(1.f,vm.ctrTextWidth));
}
void DrawText(const AnmVm& vm,float x,float y,float scale,float maxWidth,unsigned color) {
    DrawPart(vm,vm.ctrTextStart,vm.ctrTextWidth,x,y,TextWidthScale(vm,scale,maxWidth),TextHeightScale(vm,scale),color);
}

void Ascii(const char* text,float x,float y,float scale,unsigned color) {
    g_AsciiManager.scale=ZunVec2(scale,scale);g_AsciiManager.color=color;
    ZunVec3 position(x,y,0);g_AsciiManager.AddString(&position,text);
}
float AsciiWidth(const char* text,float scale) {return 14*scale*std::strlen(text);}

void Select(ResultScreen& screen,int cursor) {
    cursor=std::clamp(cursor,0,CardCount-1);
    if(cursor==screen.cursor)return;
    screen.cursor=cursor;
    if(cursor<offset)offset=cursor;
    if(cursor>=offset+CtrRecords::VisibleRows)offset=cursor-CtrRecords::VisibleRows+1;
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
}
void Page(ResultScreen& screen,int direction) {
    const int last=CardCount-CtrRecords::VisibleRows;
    const int nextOffset=std::clamp(offset+direction*CtrRecords::VisibleRows,0,last);
    const int cursor=std::clamp(int(screen.cursor)+(nextOffset-offset),0,CardCount-1);
    if(nextOffset==offset) {Select(screen,direction<0?0:CardCount-1);return;}
    offset=nextOffset;screen.cursor=cursor;
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
}

constexpr float CaptionY=CtrRecordsPanel::Margin,NameTop=136,NameScale=2;
constexpr float StatsTop=240,StatsSpacing=36,StatsScale=1.25f;
}

void CtrRecords::OpenList(ResultScreen& screen) {
    std::fill(std::begin(prepared),std::end(prepared),-1);preparedDetail=-1;
    screen.cursor=std::clamp(int(screen.cursor),0,CardCount-1);
    offset=std::clamp(offset,std::max(0,int(screen.cursor)-VisibleRows+1),std::min(int(screen.cursor),CardCount-VisibleRows));
}

bool CtrRecords::Input(ResultScreen& screen) {
    if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP))Select(screen,screen.cursor>0?screen.cursor-1:CardCount-1);
    if(WAS_PRESSED_PERIODIC(TH_BUTTON_DOWN))Select(screen,screen.cursor<CardCount-1?screen.cursor+1:0);
    if(WAS_PRESSED(TH_BUTTON_LEFT))Page(screen,-1);
    if(WAS_PRESSED(TH_BUTTON_RIGHT))Page(screen,1);
    return WAS_PRESSED(TH_BUTTON_RETURNMENU);
}

static unsigned Faded(unsigned color,float opacity) {return (unsigned((color>>24)*opacity)<<24)|(color&0xffffff);}

void CtrRecords::DrawList(ResultScreen& screen,CtrRecordsPanel::View view) {
    constexpr float scale=1.2f;
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    auto heading=screen.unk_40[13];
    heading.pos=ZunVec3(48+view.slide,64,0);
    heading.color=Faded(heading.color|0xff000000u,view.opacity);
    g_AnmManager->DrawNoRotation(&heading);
    g_AsciiManager.scale=ZunVec2(scale,scale);
    const int first=screen.cursor/VisibleRows*VisibleRows;
    for(int row=0;row<VisibleRows && first+row<CardCount;row++) {
        const int index=first+row;
        Prepare(screen,index);
        const auto color=Faded(Color(index),view.opacity);
        ZunVec3 position(12+view.slide,144+60*row,0);
        g_AsciiManager.color=color;
        g_AsciiManager.AddFormatText(&position,"No.%.2d",index+1);
        const auto& vm=screen.unk_28a0[index%RowSlots];
        DrawPart(vm,vm.ctrTextStart,vm.ctrTextWidth,position.x+90,position.y,scale,scale,color);
        position.x+=464;
        g_AsciiManager.AddFormatText(&position,"%3d/%3d",g_GameManager.catk[index].numSuccess,g_GameManager.catk[index].numAttempts);
    }
    g_AsciiManager.scale=savedScale;g_AsciiManager.color=savedColor;
}

void CtrRecords::DrawDetails(ResultScreen& screen,CtrRecordsPanel::View view) {
    const int index=screen.cursor;
    const auto& card=g_GameManager.catk[index];
    if(preparedDetail!=index) {
        auto& vm=screen.unk_28a0[DetailSlot];
        vm.fontWidth=vm.fontHeight=FitSize(Name(index));
        TextHelper::Wrapped(&vm,Name(index),COLOR_RGB(COLOR_WHITE),COLOR_RGB(COLOR_BLACK),256,true);
        preparedDetail=index;
    }
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    const auto savedGui=g_AsciiManager.isGui;const auto savedLower=g_AsciiManager.lowerScreen;
    g_AsciiManager.isGui=false;g_AsciiManager.lowerScreen=true;

    char text[32];
    std::snprintf(text,sizeof(text),"SPELL CARD No.%.2d",index+1);
    const float slide=view.LowerSlide();
    Ascii(text,CtrRecordsPanel::Left+slide,CaptionY,CtrRecordsPanel::CaptionScale,Faded(0xffffe0a0,view.opacity));

    auto* renderer=static_cast<CtrRenderer*>(g_GfxBackend);
    g_AnmManager->FlushVertexBuffer();renderer->BeginMenuLayer();
    const auto& name=screen.unk_28a0[DetailSlot];
    const float nameWidth=name.ctrTextWidth*TextWidthScale(name,NameScale,560);
    DrawText(name,320+slide-nameWidth/2,NameTop,NameScale,560,Faded(Color(index)|0xff000000u,view.opacity));
    g_AnmManager->FlushVertexBuffer();renderer->EndMenuLayer();

    char rate[8]="--";
    if(card.numAttempts)std::snprintf(rate,sizeof(rate),"%d%%",card.numSuccess*100/card.numAttempts);
    char captured[8],attempts[8];
    std::snprintf(captured,sizeof(captured),"%d",card.numSuccess);
    std::snprintf(attempts,sizeof(attempts),"%d",card.numAttempts);
    const char* labels[]={"CAPTURED","ATTEMPTS","RATE"};
    const char* values[]={captured,attempts,rate};
    // 10 characters of label and 6 of value.
    const float left=320+slide-AsciiWidth("                ",StatsScale)/2;
    for(int row=0;row<3;row++) {
        std::snprintf(text,sizeof(text),"%-10s%6s",labels[row],values[row]);
        Ascii(text,left,StatsTop+row*StatsSpacing,StatsScale,Faded(0xffe0d0d0,view.opacity));
    }

    g_AsciiManager.scale=savedScale;g_AsciiManager.color=savedColor;
    g_AsciiManager.isGui=savedGui;g_AsciiManager.lowerScreen=savedLower;
}

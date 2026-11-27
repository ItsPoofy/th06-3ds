#include "RecordsLayout.hpp"
#include "practice/Th06.hpp"
#include "Settings.hpp"
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

constexpr float ListTop=118,ListSpacing=30,ListScale=1.2f,RowHeight=24;
constexpr float BoxLeft=24,BoxRight=616,BoxMargin=10;
constexpr float CountsRight=BoxRight-16,CountsWidth=96;
constexpr float NameX=40,NameWidth=CountsRight-CountsWidth-12-NameX;
constexpr int FontSize=15;
constexpr float TextRowWidth=240;

constexpr int RowSlots=13,DigitSlot=13,DetailSlot=14;
int prepared[RowSlots];
float baselines[RowSlots];
float referenceBaseline=-1;
int preparedDetail=-1;
int offset=0;

constexpr char Digits[]="0 1 2 3 4 5 6 7 8 9 /";
constexpr int DigitCount=11;
struct Glyph {float start=0,width=0;};
Glyph digits[DigitCount];
bool digitsReady=false;

const char* Name(int index) {
    const auto& card=g_GameManager.catk[index];
    if(card.numAttempts)return card.name;
    return Practice::Th06::SpellKnown(index)?Practice::Th06::SpellName(index,!CtrSettings::English()):TH_UNKNOWN_SPELLCARD;
}
unsigned Color(int index) {
    const auto& card=g_GameManager.catk[index];
    return !Practice::Th06::SpellKnown(index)?0x80c0c0ffu:card.numSuccess==0?0xffc0a0a0u:0xfff0f0ffu;
}

std::string RowText(int index) {
    const char* name=Name(index);
    char* converted=isUTF8Encoded(name)?nullptr:sjis2utf8(name);
    std::string translated=CtrContent::Translate(converted?converted:name);
    std::free(converted);
    char number[16];
    std::snprintf(number,sizeof(number),"No.%.2d  ",index+1);
    return number+translated;
}

int FitSize(const char* text) {
    int size=FontSize;
    while(size>8 && TextHelper::Measure(text,size)>TextRowWidth)size--;
    return size;
}

float FirstGlyphBottom(const AnmVm& vm) {
    const auto& texture=g_AnmManager->textures[vm.sprite->sourceFileIndex];
    if(!texture.textureData)return -1;
    const int row=vm.sprite->startPixelInclusive.y;
    const int bottom=std::min<int>(row+int(vm.ctrTextHeight),texture.height);
    int last=-1;bool started=false;
    for(unsigned x=unsigned(vm.ctrTextStart);x<texture.width;x++) {
        bool ink=false;
        for(int y=row;y<bottom;y++) {
            const u8* pixel=&texture.textureData[(y*texture.width+x)*4];
            if(pixel[3]>128 && pixel[0]>128) {ink=true;last=std::max(last,y);}
        }
        if(ink)started=true;else if(started)break;
    }
    return last<0?-1:float(last+1-row);
}

void Prepare(ResultScreen& screen,int index) {
    const int slot=index%RowSlots;
    if(prepared[slot]==index)return;
    auto& vm=screen.unk_28a0[slot];
    const std::string row=RowText(index);
    vm.fontWidth=vm.fontHeight=FitSize(row.c_str());
    TextHelper::Wrapped(&vm,row.c_str(),COLOR_RGB(COLOR_WHITE),COLOR_RGB(COLOR_BLACK),256,true);
    baselines[slot]=FirstGlyphBottom(vm);
    if(vm.fontHeight==FontSize && baselines[slot]>=0)referenceBaseline=baselines[slot];
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
    g_AnmManager->SetCurrentSprite(nullptr);
}

float TextHeightScale(const AnmVm& vm,float scale) {return scale*FontSize/(vm.fontHeight>0?vm.fontHeight:FontSize);}
float TextWidthScale(const AnmVm& vm,float scale,float maxWidth) {
    return std::min(TextHeightScale(vm,scale),maxWidth/std::max(1.f,vm.ctrTextWidth));
}
void DrawText(const AnmVm& vm,float x,float y,float scale,float maxWidth,unsigned color) {
    DrawPart(vm,vm.ctrTextStart,vm.ctrTextWidth,x,y,TextWidthScale(vm,scale,maxWidth),TextHeightScale(vm,scale),color);
}

void PrepareDigits(ResultScreen& screen) {
    if(digitsReady)return;
    auto& vm=screen.unk_28a0[DigitSlot];
    vm.fontWidth=vm.fontHeight=FontSize;
    TextHelper::Wrapped(&vm,Digits,COLOR_RGB(COLOR_WHITE),COLOR_RGB(COLOR_BLACK),256,true);
    const auto& texture=g_AnmManager->textures[vm.sprite->sourceFileIndex];
    if(!texture.textureData)return;
    const int row=vm.sprite->startPixelInclusive.y;
    const int bottom=std::min<int>(row+int(vm.ctrTextHeight),texture.height);
    int count=0;bool inside=false;
    for(unsigned x=0;x<texture.width && count<=DigitCount;x++) {
        bool ink=false;
        for(int y=row;y<bottom && !ink;y++)ink=texture.textureData[(y*texture.width+x)*4+3]!=0;
        if(ink && !inside) {if(count<DigitCount)digits[count].start=x;inside=true;}
        if(!ink && inside) {if(count<DigitCount)digits[count].width=x-digits[count].start;count++;inside=false;}
    }
    digitsReady=count==DigitCount;
}

void DrawCounts(ResultScreen& screen,int index,float right,float y,unsigned color) {
    if(!digitsReady)return;
    const auto& card=g_GameManager.catk[index];
    char text[16];
    std::snprintf(text,sizeof(text),"%d/%d",card.numSuccess,card.numAttempts);
    const float gap=digits[0].width*.1f;
    float width=0;
    for(const char* c=text;*c;c++) {
        const auto& glyph=digits[*c=='/'?10:*c-'0'];
        width+=(glyph.width+gap)*ListScale;
    }
    float x=right-width;
    const auto& vm=screen.unk_28a0[DigitSlot];
    for(const char* c=text;*c;c++) {
        const auto& glyph=digits[*c=='/'?10:*c-'0'];
        DrawPart(vm,glyph.start,glyph.width,x,y,ListScale,ListScale,color);
        x+=(glyph.width+gap)*ListScale;
    }
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
    std::fill(std::begin(prepared),std::end(prepared),-1);preparedDetail=-1;digitsReady=false;referenceBaseline=-1;
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
    const int end=std::min(CardCount,offset+VisibleRows);
    const float slide=view.slide;
    CtrPresentation::ShowListBox(BoxLeft+slide,ListTop-BoxMargin,BoxRight+slide,ListTop+(VisibleRows-1)*ListSpacing+RowHeight+BoxMargin,
                                 offset>0,end<CardCount,view.opacity);
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    PrepareDigits(screen);
    for(int index=offset;index<end;index++) {
        Prepare(screen,index);
        unsigned color=Color(index);
        color=index==screen.cursor?color|0xff000000u:(color&0xffffff)|((color>>24)*9/16<<24);
        color=Faded(color,view.opacity);
        const float y=ListTop+(index-offset)*ListSpacing;
        const int slot=index%RowSlots;
        const auto& vm=screen.unk_28a0[slot];
        const float shift=referenceBaseline>=0 && baselines[slot]>=0?
            referenceBaseline*ListScale-baselines[slot]*TextHeightScale(vm,ListScale):0;
        DrawText(vm,NameX+slide,y+shift,ListScale,NameWidth,color);
        DrawCounts(screen,index,CountsRight+slide,y,color);
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

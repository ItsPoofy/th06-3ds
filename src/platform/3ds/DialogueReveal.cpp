#include "DialogueReveal.hpp"
#include "TextHelper.hpp"
#include <array>
#include <algorithm>
namespace {
struct Line {std::array<float,256> widths{};int count=0,visible=0;};
Line lines[CtrDialogue::Capacity];
// ANM files use texture slots 0..127. Keep the extra dialogue canvas separate
// so it cannot overwrite intro, spell, record or ending text in text.anm.
constexpr int overflowTexture=263;
AnmLoadedSprite overflowSprite{};
bool active=false;
int frame=0,lineCount=0,readingFrames=0;
}
void CtrDialogue::BindOverflow(AnmVm& vm) {
    if(!g_AnmManager->textures[overflowTexture].handle)
        g_AnmManager->CreateEmptyTexture(overflowTexture,512,32,TEX_FMT_A8R8G8B8);
    overflowSprite=*vm.sprite;
    overflowSprite.sourceFileIndex=overflowTexture;
    overflowSprite.startPixelInclusive=ZunVec2(0,0);
    overflowSprite.endPixelInclusive=ZunVec2(512,32);
    overflowSprite.textureWidth=512;overflowSprite.textureHeight=32;
    overflowSprite.widthPx=512;overflowSprite.heightPx=32;
    overflowSprite.uvStart=ZunVec2(.5f/512,.5f/32);
    overflowSprite.uvEnd=ZunVec2(511.5f/512,31.5f/32);
    vm.sprite=&overflowSprite;
}
void CtrDialogue::ReleaseOverflow() {g_AnmManager->ReleaseTexture(overflowTexture);Reset();}
int CtrDialogue::LineCount() {return lineCount;}
void CtrDialogue::Reset() {for(auto& line:lines)line={};active=false;frame=0;lineCount=0;readingFrames=0;}
void CtrDialogue::StartLine(int index,const char* text,int fontHeight) {
    if(index<0 || index>=Capacity)return;
    if(index==0)Reset();
    lineCount=std::max(lineCount,index+1);
    active=true;auto& line=lines[index];line={};
    // Measure once at line creation. Revealing only crops the existing GPU
    // texture; no font rendering, allocation or texture upload occurs per tick.
    line.count=TextHelper::PrefixWidths(text,fontHeight,line.widths.data(),line.widths.size());
}
void CtrDialogue::Tick() {
    if(!active)return;
    if(!Revealing()){++readingFrames;return;}
    if(++frame%2)return;
    for(auto& line:lines)if(line.visible<line.count){++line.visible;break;}
}
bool CtrDialogue::Active() {return active;}
bool CtrDialogue::Revealing() {for(const auto& line:lines)if(line.visible<line.count)return true;return false;}
bool CtrDialogue::AutoAdvanceReady() {
    int letters=0;for(const auto& line:lines)letters+=line.count;
    return !active || (!Revealing() && readingFrames>=std::max(90,letters));
}
void CtrDialogue::Finish() {for(auto& line:lines)line.visible=line.count;}
float CtrDialogue::Width(int index,float fullWidth) {
    if(!active || index<0 || index>=Capacity)return fullWidth;
    const auto& line=lines[index];
    return line.visible==line.count?fullWidth:std::min(fullWidth,line.widths[line.visible]);
}

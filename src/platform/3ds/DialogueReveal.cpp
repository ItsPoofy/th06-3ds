#include "DialogueReveal.hpp"
#include "TextHelper.hpp"
#include <array>
#include <algorithm>
namespace {
struct Line {std::array<float,256> widths{};int count=0,visible=0;};
Line lines[2];
bool active=false;
int frame=0;
}
void CtrDialogue::Reset() {for(auto& line:lines)line={};active=false;frame=0;}
void CtrDialogue::StartLine(int index,const char* text,int fontHeight) {
    if(index<0 || index>1)return;
    if(index==0)Reset();
    active=true;auto& line=lines[index];line={};
    // Measure once at line creation. Revealing only crops the existing GPU
    // texture; no font rendering, allocation or texture upload occurs per tick.
    line.count=TextHelper::PrefixWidths(text,fontHeight,line.widths.data(),line.widths.size());
}
void CtrDialogue::Tick() {
    if(!active || ++frame%2)return;
    for(auto& line:lines)if(line.visible<line.count){++line.visible;break;}
}
bool CtrDialogue::Active() {return active;}
bool CtrDialogue::Revealing() {for(const auto& line:lines)if(line.visible<line.count)return true;return false;}
void CtrDialogue::Finish() {for(auto& line:lines)line.visible=line.count;}
float CtrDialogue::Width(int index,float fullWidth) {
    if(!active || index<0 || index>1)return fullWidth;
    const auto& line=lines[index];
    return line.visible==line.count?fullWidth:std::min(fullWidth,line.widths[line.visible]);
}

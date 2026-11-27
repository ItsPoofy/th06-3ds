#pragma once
#include "NativeImage.hpp"
namespace CtrMenuCaption {
inline constexpr float Scale=.5f;
inline constexpr unsigned Active=0xffe0a0,Idle=0x968282;
inline void Draw(C2D_Image image,float x,float y,bool active,unsigned alpha) {
    const unsigned tint=active?Active:Idle;
    CtrNative::DrawImage(image,x,y,C2D_Color32(tint>>16,(tint>>8)&255,tint&255,alpha),Scale,Scale);
}
inline void Rule(float left,float top,float labelWidth,float right,unsigned alpha) {
    const float start=left+labelWidth+6;
    if(right>start)C2D_DrawRectSolid(start,top+5,0,right-start,1,C2D_Color32(120,60,60,alpha));
}
}

#pragma once
#include <citro2d.h>
#include <iterator>

namespace CtrMenuBackdrop {
inline void Draw() {
    // Sampled from the mockup's lower panel at the native 320x240 size.
    constexpr int rows[]={0,11,22,33,44,54,65,76,87,98,109,120,
                          131,142,152,163,174,185,196,207,218,229,240};
    constexpr int topRed=27;
    for(unsigned i=0;i+1<std::size(rows);i++)
        C2D_DrawRectSolid(0,rows[i],0,320,rows[i+1]-rows[i],C2D_Color32(topRed-i,0,0,255));
}
}

#pragma once
#include <SDL2/SDL_ttf.h>

namespace StageCaptionSurface {
    inline constexpr int RowHeight=32,RowStep=40;
    SDL_Surface* RenderLine(TTF_Font* font,const char* text,SDL_Color color,int minimumHeight);
    SDL_Surface* WhiteOutlined(SDL_Surface* source,int radius);
    // Width is measured at the renderer's doubled font resolution.
    SDL_Surface* Render(TTF_Font* font,const char* text,SDL_Color color,bool wrap,int maxWidth,bool centered=true,bool rightAligned=false);
}

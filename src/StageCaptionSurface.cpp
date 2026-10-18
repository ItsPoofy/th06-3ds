#include "StageCaptionSurface.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string Trim(const std::string& text) {
    const auto first=text.find_first_not_of(' '),last=text.find_last_not_of(' ');
    return first==std::string::npos?"":text.substr(first,last-first+1);
}
std::vector<std::string> Wrap(TTF_Font* font,const std::string& text,int width) {
    std::vector<std::string> rows;
    std::istringstream words(text);std::string word,line;
    while(words>>word) {
        const auto candidate=line.empty()?word:line+" "+word;
        int measured=0,height=0;TTF_SizeUTF8(font,candidate.c_str(),&measured,&height);
        if(measured<=width) {line=candidate;continue;}
        if(!line.empty()) {rows.push_back(line);line.clear();}
        // A long token (including unspaced Japanese) still needs a boundary.
        for(size_t i=0;i<word.size();) {
            const unsigned char first=word[i];
            const size_t bytes=first<0x80?1:first<0xe0?2:first<0xf0?3:4;
            const auto glyph=word.substr(i,bytes);i+=bytes;
            TTF_SizeUTF8(font,(line+glyph).c_str(),&measured,&height);
            if(!line.empty() && measured>width) {rows.push_back(line);line.clear();}
            line+=glyph;
        }
    }
    if(!line.empty())rows.push_back(line);
    return rows;
}
SDL_Rect InkBounds(SDL_Surface* surface) {
    int left=surface->w,top=surface->h,right=0,bottom=0;
    for(int y=0;y<surface->h;y++)for(int x=0;x<surface->w;x++) {
        Uint32 pixel;std::memcpy(&pixel,static_cast<Uint8*>(surface->pixels)+y*surface->pitch+x*4,4);
        if(pixel&surface->format->Amask) {
            left=std::min(left,x);right=std::max(right,x+1);
            top=std::min(top,y);bottom=std::max(bottom,y+1);
        }
    }
    return {left,top,std::max(0,right-left),std::max(0,bottom-top)};
}
}

SDL_Surface* StageCaptionSurface::WhiteOutlined(SDL_Surface* source,int radius) {
    if(!source)return nullptr;
    auto* output=SDL_CreateRGBSurfaceWithFormat(0,source->w+2*radius,source->h+2*radius,32,SDL_PIXELFORMAT_RGBA32);
    if(!output)return nullptr;
    SDL_FillRect(output,nullptr,0);
    std::vector<Uint8> sourceAlpha(source->w*source->h);
    for(int y=0;y<source->h;y++)for(int x=0;x<source->w;x++) {
        Uint32 pixel;Uint8 r,g,b,a;
        std::memcpy(&pixel,static_cast<Uint8*>(source->pixels)+y*source->pitch+x*4,4);
        SDL_GetRGBA(pixel,source->format,&r,&g,&b,&a);sourceAlpha[y*source->w+x]=a;
    }
    auto alphaAt=[&](int x,int y) {
        if(x<0 || y<0 || x>=source->w || y>=source->h)return 0;
        return int(sourceAlpha[y*source->w+x]);
    };
    for(int y=0;y<output->h;y++)for(int x=0;x<output->w;x++) {
        const int sx=x-radius,sy=y-radius;
        int outline=0;
        for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++)
            if(dx*dx+dy*dy<=radius*radius)outline=std::max(outline,alphaAt(sx+dx,sy+dy));
        const int fill=alphaAt(sx,sy),alpha=fill+outline*(255-fill)/255;
        const Uint8 value=alpha?255*fill/alpha:0;
        const Uint32 pixel=SDL_MapRGBA(output->format,value,value,value,alpha);
        std::memcpy(static_cast<Uint8*>(output->pixels)+y*output->pitch+x*4,&pixel,4);
    }
    const auto ink=InkBounds(output);
    if(ink.w==0 || ink.h==0)return output;
    auto* cropped=SDL_CreateRGBSurfaceWithFormat(0,ink.w,ink.h,32,SDL_PIXELFORMAT_RGBA32);
    if(!cropped)return output;
    SDL_SetSurfaceBlendMode(output,SDL_BLENDMODE_NONE);
    SDL_Rect src=ink;SDL_BlitSurface(output,&src,cropped,nullptr);
    SDL_FreeSurface(output);return cropped;
}

SDL_Surface* StageCaptionSurface::RenderLine(TTF_Font* font,const char* text,SDL_Color color,int minimumHeight) {
    auto* source=TTF_RenderUTF8_Blended(font,text,color);
    if(!source)return nullptr;
    const auto ink=InkBounds(source);
    const int height=std::max(minimumHeight,ink.h+2);
    auto* output=SDL_CreateRGBSurfaceWithFormat(0,source->w,height,32,SDL_PIXELFORMAT_RGBA32);
    if(output) {
        SDL_FillRect(output,nullptr,0);
        SDL_Rect crop{0,ink.y,source->w,ink.h};
        SDL_Rect dst{0,(height-ink.h)/2,0,0};
        SDL_SetSurfaceBlendMode(source,SDL_BLENDMODE_NONE);
        SDL_BlitSurface(source,&crop,output,&dst);
    }
    SDL_FreeSurface(source);
    return output;
}

SDL_Surface* StageCaptionSurface::Render(TTF_Font* font,const char* input,SDL_Color color,bool english,int maxWidth,bool centered,bool rightAligned) {
    std::string text(input);
    for(const char* token:{"\xef\xbd\x9e","\xe3\x80\x9c","\xe2\x88\xbc"})
        for(size_t p=0;(p=text.find(token,p))!=std::string::npos;)text.replace(p,std::strlen(token),"~");
    for(size_t p=0;(p=text.find("\xe3\x80\x80",p))!=std::string::npos;)text.replace(p,3," ");
    std::vector<std::string> rows;
    if(english) {
        const auto separator=text.find('~');
        rows=Wrap(font,Trim(text.substr(0,separator)),maxWidth);
        if(separator!=std::string::npos) {
            auto subtitle=Wrap(font,"~ "+Trim(text.substr(separator+1))+" ~",maxWidth);
            rows.insert(rows.end(),subtitle.begin(),subtitle.end());
        }
    } else rows.push_back(Trim(text));
    if(rows.empty())return nullptr;
    std::vector<SDL_Surface*> surfaces(rows.size());std::vector<SDL_Rect> bounds(rows.size());
    int width=1,rowHeight=english?40:RowHeight;
    for(size_t i=0;i<rows.size();i++) {
        surfaces[i]=TTF_RenderUTF8_Blended(font,rows[i].c_str(),color);
        if(surfaces[i]) {bounds[i]=InkBounds(surfaces[i]);width=std::max(width,bounds[i].w);rowHeight=std::max(rowHeight,bounds[i].h);}
    }
    const int rowStep=rowHeight+8;
    auto* output=SDL_CreateRGBSurfaceWithFormat(0,width,rowStep*(rows.size()-1)+rowHeight,32,SDL_PIXELFORMAT_RGBA32);
    if(output) {
        SDL_FillRect(output,nullptr,0);
        for(size_t i=0;i<surfaces.size();i++)if(surfaces[i]) {
            SDL_Rect dst{rightAligned?width-bounds[i].w:centered?(width-bounds[i].w)/2:0,int(i)*rowStep+(rowHeight-bounds[i].h)/2,0,0};
            SDL_SetSurfaceBlendMode(surfaces[i],SDL_BLENDMODE_NONE);
            SDL_BlitSurface(surfaces[i],&bounds[i],output,&dst);
        }
    }
    for(auto* surface:surfaces)SDL_FreeSurface(surface);
    return output;
}

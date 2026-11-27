#include "ReplayNames.hpp"
#include "PackedTexture.hpp"
#include "TiledTexture.hpp"
#include "NativeImage.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>
namespace {
constexpr int Oversample=3,DisplayScale=2,Outline=2*Oversample;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
struct Label {
    CtrAssets::PackedTexture texture;
    std::string name;
    int size=0,maxWidth=0,stamp=0,left=0,top=0,width=0,height=0;
};
std::array<Label,18> labels;
TTF_Font* fonts[2]{};
int stamp=0;
void Synchronize(){if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();}
TTF_Font* Font(int size) {
    const int index=size==12?0:1;
    if(!fonts[index]) {
        fonts[index]=TTF_OpenFont(TH06_ASSET_ROOT "ui/replay-name.ttf",size*DisplayScale*Oversample);
        if(fonts[index]){TTF_SetFontHinting(fonts[index],TTF_HINTING_LIGHT);TTF_SetFontKerning(fonts[index],1);}
    }
    return fonts[index];
}
Label* Find(const char* name,int size,int maxWidth) {
    for(auto& label:labels)if(label.texture.ready && label.name==name && label.size==size && label.maxWidth==maxWidth)return &label;
    return nullptr;
}
Surface Render(TTF_Font* font,const std::string& text,SDL_Color color) {
    Surface raw(TTF_RenderUTF8_Blended(font,text.c_str(),color),SDL_FreeSurface);
    return Surface(raw?SDL_ConvertSurfaceFormat(raw.get(),SDL_PIXELFORMAT_RGBA32,0):nullptr,SDL_FreeSurface);
}
void Composite(std::vector<unsigned char>& pixels,int width,int height,const SDL_Surface* source,int x,int y) {
    for(int row=0;row<source->h && y+row<height;row++)for(int col=0;col<source->w && x+col<width;col++) {
        const auto* src=static_cast<const unsigned char*>(source->pixels)+row*source->pitch+col*4;
        auto* dst=pixels.data()+((y+row)*width+x+col)*4;
        const unsigned alpha=src[3]+(dst[3]*(255-src[3])+127)/255;
        if(!alpha)continue;
        for(int channel=0;channel<3;channel++)dst[channel]=(src[channel]*src[3]+(dst[channel]*dst[3]*(255-src[3])+127)/255+alpha/2)/alpha;
        dst[3]=alpha;
    }
}
bool Raster(Label& label,TTF_Font* font,std::string text) {
    int measured=0;
    TTF_SizeUTF8(font,text.c_str(),&measured,nullptr);
    if(measured>label.maxWidth*DisplayScale*Oversample) {
        do {text.pop_back();TTF_SizeUTF8(font,(text+"...").c_str(),&measured,nullptr);}
        while(!text.empty() && measured>label.maxWidth*DisplayScale*Oversample);
        text+="...";
    }
    TTF_SetFontOutline(font,Outline);
    auto outline=Render(font,text,SDL_Color{0,0,0,255});
    TTF_SetFontOutline(font,0);
    auto face=Render(font,text,SDL_Color{255,255,255,255});
    if(!outline || !face)return false;
    const int highWidth=outline->w,highHeight=outline->h;
    std::vector<unsigned char> high(highWidth*highHeight*4);
    Composite(high,highWidth,highHeight,outline.get(),0,0);
    Composite(high,highWidth,highHeight,face.get(),Outline,Outline);
    const int width=(highWidth+Oversample-1)/Oversample,height=(highHeight+Oversample-1)/Oversample;
    int powerWidth=8,powerHeight=8;
    while(powerWidth<width)powerWidth*=2;
    while(powerHeight<height)powerHeight*=2;
    if(powerWidth>1024 || powerHeight>1024 || !C3D_TexInit(&label.texture.gpu,powerWidth,powerHeight,GPU_RGBA8))return false;
    label.texture.ready=true;std::memset(label.texture.gpu.data,0,label.texture.gpu.size);
    int left=width,top=height,right=-1,bottom=-1;
    for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
        unsigned alpha=0,red=0,green=0,blue=0,faceAlpha=0;
        for(int dy=0;dy<Oversample;dy++)for(int dx=0;dx<Oversample;dx++) {
            const int hx=x*Oversample+dx,hy=y*Oversample+dy;
            if(hx>=highWidth || hy>=highHeight)continue;
            const auto* pixel=high.data()+(hy*highWidth+hx)*4;
            alpha+=pixel[3];red+=pixel[0]*pixel[3];green+=pixel[1]*pixel[3];blue+=pixel[2]*pixel[3];
            const int fx=hx-Outline,fy=hy-Outline;
            if(fx>=0 && fy>=0 && fx<face->w && fy<face->h)
                faceAlpha+=*(static_cast<const unsigned char*>(face->pixels)+fy*face->pitch+fx*4+3);
        }
        auto* out=static_cast<unsigned char*>(label.texture.gpu.data)+CtrTexture::TexelOffset(x,y,powerWidth)*4;
        out[0]=(alpha+4)/9;
        if(alpha){out[1]=(blue+alpha/2)/alpha;out[2]=(green+alpha/2)/alpha;out[3]=(red+alpha/2)/alpha;}
        if(faceAlpha>=128*Oversample*Oversample){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x);bottom=std::max(bottom,y);}
    }
    if(right<left){label.texture.Clear();return false;}
    label.left=left;label.top=top;label.width=right-left+1;label.height=bottom-top+1;
    label.texture.sub={u16(width),u16(height),0,1,float(width)/powerWidth,1-float(height)/powerHeight};
    C3D_TexSetFilter(&label.texture.gpu,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&label.texture.gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);C3D_TexFlush(&label.texture.gpu);
    return true;
}
}
bool CtrReplayNames::Prepare(const char* name,int size,int maxWidth) {
    if(!*name || (size!=12 && size!=15))return false;
    if(auto* label=Find(name,size,maxWidth)){label->stamp=++stamp;return true;}
    auto* font=Font(size);if(!font)return false;
    auto* label=&*std::min_element(labels.begin(),labels.end(),[](const auto& a,const auto& b){return a.stamp<b.stamp;});
    if(label->texture.ready){Synchronize();label->texture.Clear();}
    label->name=name;label->size=size;label->maxWidth=maxWidth;label->stamp=++stamp;
    return Raster(*label,font,name);
}
void CtrReplayNames::Draw(const char* name,float x,float centerY,int size,int maxWidth,unsigned color,bool centered) {
    auto* label=Find(name,size,maxWidth);if(!label)return;
    label->stamp=++stamp;
    const float left=std::round(x-(label->left+(centered?label->width*.5f:0))/DisplayScale);
    const float top=std::round(centerY-(label->top+label->height*.5f)/DisplayScale);
    CtrNative::DrawImage({&label->texture.gpu,&label->texture.sub},left,top,color,.5f,.5f);
}
void CtrReplayNames::Release() {
    if(std::any_of(labels.begin(),labels.end(),[](const auto& label){return label.texture.ready;}))Synchronize();
    for(auto& label:labels){label.texture.Clear();label.name.clear();label.stamp=0;}
    for(auto*& font:fonts)if(font){TTF_CloseFont(font);font=nullptr;}
    stamp=0;
}

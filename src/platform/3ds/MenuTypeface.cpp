#include "MenuTypeface.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "Settings.hpp"
#include "../../../assets/ui-font/menu_typeface.hpp"
#include <algorithm>
#include <iterator>

namespace {
using namespace CtrMenuTypefaceAssets;
CtrAssets::PackedTexture latin,japanese;
// One load attempt per menu visit, so a missing sheet is not reopened per glyph.
bool japaneseTried=false;
void LoadJapanese() {
    if(japanese.ready || japaneseTried || CtrSettings::English())return;
    japaneseTried=true;japanese.Load(TH06_ASSET_ROOT "ui/menu-typeface-ja.bin");
}
// Reads one UTF-8 code point and advances text past it.
unsigned Next(const unsigned char*& text) {
    const unsigned lead=*text++;
    if(lead<0x80)return lead;
    const int extra=lead>=0xf0?3:lead>=0xe0?2:1;
    unsigned code=lead&(0x3f>>extra);
    for(int i=0;i<extra && (*text&0xc0)==0x80;i++)code=code<<6|(*text++&0x3f);
    return code;
}
// The glyph for a code point and its atlas; ASCII from the Latin sheet, the
// rest from the Japanese one. Characters the sheets lack are skipped.
const Glyph* Find(unsigned code,CtrAssets::PackedTexture*& atlas) {
    if(code>=32 && code<=126){atlas=&latin;return &glyphs[code-32];}
    LoadJapanese();
    const auto* end=std::end(jaCodes);
    const auto* found=std::lower_bound(std::begin(jaCodes),end,code);
    if(found==end || *found!=code)return nullptr;
    atlas=&japanese;return &jaGlyphs[found-std::begin(jaCodes)];
}
}
// The Japanese sheet (2 MB) loads only while the game is in Japanese.
bool CtrMenuTypeface::Initialize() {
    LoadJapanese();
    return latin.ready || latin.Load(TH06_ASSET_ROOT "ui/menu-typeface.bin");
}
void CtrMenuTypeface::Shutdown(){latin.Clear();japanese.Clear();japaneseTried=false;}
float CtrMenuTypeface::Width(const char* text,float height) {
    float width=0;CtrAssets::PackedTexture* atlas=nullptr;
    for(const auto* c=reinterpret_cast<const unsigned char*>(text);*c;)
        if(const auto* g=Find(Next(c),atlas))width+=g->advance/64.f;
    return width*height/size;
}
// Each glyph carries its own shading, stroke and shadow, so a white color keeps
// them as drawn and another color tints the face (as the selected Options row
// does). Every stroke is drawn before any face, as in a label rendered whole.
void CtrMenuTypeface::Draw(const char* text,float x,float y,float height,unsigned color,float maxWidth,bool centered) {
    if(!latin.ready || !(color>>24))return;
    const float width=Width(text,height),sy=height/size,sx=sy*std::min(1.f,maxWidth/std::max(1.f,width));
    if(centered)x-=std::min(width,maxWidth)/2;
    for(int layer=0;layer<2;layer++) {
        float pen=x;
        for(const auto* c=reinterpret_cast<const unsigned char*>(text);*c;) {
            CtrAssets::PackedTexture* atlas=nullptr;
            const auto* g=Find(Next(c),atlas);
            if(!g)continue;
            if(g->w && atlas->ready) {
                const int gx=layer?g->faceX:g->x,gy=layer?g->faceY:g->y;
                const float w=atlas->gpu.width,h=atlas->gpu.height;
                Tex3DS_SubTexture sub{u16(g->w),u16(g->h),gx/w,1-gy/h,(gx+g->w)/w,1-(gy+g->h)/h};
                CtrNative::DrawImage({&atlas->gpu,&sub},pen+g->left*sx,y+g->top*sy,color,sx,sy);
            }
            pen+=g->advance*sx/64;
        }
    }
}
void CtrMenuTypeface::DrawMode(int mode,float centerX,float centerY,unsigned opacity,float scale) {
    const bool english=CtrSettings::English();
    LoadJapanese();
    auto& atlas=english?latin:japanese;
    if(!atlas.ready || !opacity)return;
    const float top=(english?modeTop:jaModeTop)+(modeHeight+2)*mode;
    const float w=atlas.gpu.width,h=atlas.gpu.height;
    Tex3DS_SubTexture sub{u16(modeWidth),u16(modeHeight),0,1-top/h,modeWidth/w,1-(top+modeHeight)/h};
    CtrNative::DrawImage({&atlas.gpu,&sub},centerX-modeWidth*.5f*scale,centerY-modeHeight*.5f*scale,C2D_Color32(255,255,255,opacity),scale,scale);
}

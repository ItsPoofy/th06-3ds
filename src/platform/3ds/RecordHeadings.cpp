#include "RecordHeadings.hpp"
#include "PackedTexture.hpp"
#include "ContentAssets.hpp"
#include "Settings.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
#include "NativeImage.hpp"
#include "MenuCaption.hpp"
#include "../../../assets/record_headings.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
namespace {
CtrAssets::PackedTexture atlas;
bool english=false;
}
void CtrRecordHeadings::Release() {
    if(atlas.ready && g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    atlas.Clear();
}
bool CtrRecordHeadings::Prepare() {
    if(!atlas.ready || english!=CtrSettings::English()) {
        Release();english=CtrSettings::English();
        if(!atlas.Load(CtrContent::Path("ui","record-headings.bin").c_str()))return false;
    }
    return true;
}
void CtrRecordHeadings::Draw(const char* name,float x,float y,unsigned color,float maxWidth,bool centered) {
    if(!atlas.ready)return;
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name)) {
        const int width=english?label.en:label.jp;
        const float scale=std::min(1.f,maxWidth*label.divisor/width)/label.divisor,w=atlas.gpu.width,h=atlas.gpu.height;
        Tex3DS_SubTexture sub{u16(width),u16(label.h),label.x/w,1-label.y/h,(label.x+width)/w,1-(label.y+label.h)/h};
        CtrNative::DrawImage({&atlas.gpu,&sub},centered?x-width*scale/2:x,y,color,scale,scale);
        return;
    }
}
float CtrRecordHeadings::TextWidth(const char* name) {
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name))
        return float(english?label.enInkWidth:label.jpInkWidth)/label.divisor;
    return 0;
}
float CtrRecordHeadings::TextHeight(const char* name) {
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name))
        return float(english?label.enInkHeight:label.jpInkHeight)/label.divisor;
    return 0;
}
float CtrRecordHeadings::CellWidth(const char* name) {
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name))
        return float(english?label.en:label.jp)/label.divisor;
    return 0;
}
float CtrRecordHeadings::TextAdvance(const char* name) {
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name))
        return float(english?label.enAdvance:label.jpAdvance)/(64*label.divisor);
    return 0;
}
void CtrRecordHeadings::DrawCaption(const char* name,float x,float y,bool active,float opacity) {
    if(!atlas.ready)return;
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name)) {
        const int width=english?label.en:label.jp;
        const float w=atlas.gpu.width,h=atlas.gpu.height;
        Tex3DS_SubTexture sub{u16(width),u16(label.h),label.x/w,1-label.y/h,(label.x+width)/w,1-(label.y+label.h)/h};
        CtrMenuCaption::Draw({&atlas.gpu,&sub},x,y,active,unsigned(255*opacity));
        return;
    }
}
void CtrRecordHeadings::DrawCaptionAtInk(const char* name,float x,float y,bool active,float opacity) {
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name)) {
        const float left=float(english?label.enLeft:label.jpLeft)/label.divisor;
        const float top=float(english?label.enTop:label.jpTop)/label.divisor;
        DrawCaption(name,std::round(x-left),std::round(y-top),active,opacity);
        return;
    }
}
void CtrRecordHeadings::DrawText(const char* name,float x,float y,unsigned color,bool centered) {
    if(!atlas.ready)return;
    for(const auto& label:CtrRecordHeadingAssets::labels)if(!std::strcmp(name,label.name)) {
        const int width=english?label.en:label.jp;
        const int left=english?label.enLeft:label.jpLeft,top=english?label.enTop:label.jpTop;
        const float scale=1.f/label.divisor;
        const float inkWidth=english?label.enInkWidth:label.jpInkWidth;
        const float w=atlas.gpu.width,h=atlas.gpu.height;
        Tex3DS_SubTexture sub{u16(width),u16(label.h),label.x/w,1-label.y/h,(label.x+width)/w,1-(label.y+label.h)/h};
        CtrNative::DrawImage({&atlas.gpu,&sub},std::round(x-(left+(centered?inkWidth/2:0))*scale),std::round(y-top*scale),color,scale,scale);
        return;
    }
}

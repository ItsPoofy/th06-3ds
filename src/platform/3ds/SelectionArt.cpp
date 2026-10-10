#include "SelectionArt.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "Settings.hpp"
#include "ContentAssets.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
#include "../../../assets/selection_labels.hpp"
#include "../../../assets/selection_en_labels.hpp"
#include <algorithm>
#include <cstring>
#include <string_view>
#include <unordered_map>
namespace {
CtrAssets::PackedTexture atlas;
bool atlasEnglish=false;
const CtrSelectionAssets::Label* Find(const char* name) {
    static const auto labels=[] {
        std::unordered_map<std::string_view,const CtrSelectionAssets::Label*> result;
        result.reserve(std::size(CtrSelectionAssets::labels));
        for(const auto& label:CtrSelectionAssets::labels)result.emplace(label.name,&label);
        return result;
    }();
    static const auto english=[] {
        std::unordered_map<std::string_view,const CtrSelectionAssets::Label*> result;
        for(const auto& label:CtrSelectionEnglishAssets::labels)result.emplace(label.name,&label);
        return result;
    }();
    const auto& lookup=atlasEnglish?english:labels;
    const auto found=lookup.find(name);return found==lookup.end()?nullptr:found->second;
}
}
bool CtrSelectionArt::Initialize() {Find("hud-digit0");atlasEnglish=CtrSettings::English();return atlas.Load(CtrContent::Path("ui","selection-atlas.bin").c_str());}
void CtrSelectionArt::RefreshLanguage() {
    if(atlasEnglish==CtrSettings::English())return;
    if(g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    Initialize();
}
void CtrSelectionArt::Shutdown() {atlas.Clear();}
const char* CtrSelectionArt::DifficultyName(int d) {static const char* names[]={"Easy","Normal","Hard","Lunatic","Extra"};return names[std::clamp(d,0,4)];}
unsigned CtrSelectionArt::DifficultyColor(int d,unsigned a) {
    static const unsigned colors[]={0x80ff80,0x80c0ff,0xf080ff,0xff8080,0xffdb60};
    const auto c=colors[std::clamp(d,0,4)];return C2D_Color32(c>>16,(c>>8)&255,c&255,a);
}
void CtrSelectionArt::Draw(const char* name,float x,float y,unsigned tint,bool centered,float scale) {
    if(!atlas.ready)return;
    // Difficulty and Stage labels already contain the sampled white-to-color
    // face. Preserve their white highlight instead of multiplying it by RGB.
    std::string coloredName;
    static const unsigned colors[]={0x80ff80,0x80c0ff,0xf080ff,0xff8080,0xffdb60};
    bool difficultyLabel=false;
    for(int i=0;i<5;i++)if(!strcmp(name,DifficultyName(i)))difficultyLabel=true;
    if(!strncmp(name,"Stage ",6))for(int i=0;i<5;i++) {
        const unsigned rgb=C2D_Color32(colors[i]>>16,(colors[i]>>8)&255,colors[i]&255,255)&0xffffff;
        if((tint&0xffffff)==rgb) {coloredName=std::string(name)+".difficulty"+std::to_string(i);name=coloredName.c_str();difficultyLabel=true;break;}
    }
    if(difficultyLabel)tint=(tint&0xff000000)|0xffffff;
    if(const auto* label=Find(name)) {const auto& l=*label;
        Tex3DS_SubTexture sub{u16(l.w),u16(l.h),float(l.x)/atlas.gpu.width,1-float(l.y)/atlas.gpu.height,
            float(l.x+l.w)/atlas.gpu.width,1-float(l.y+l.h)/atlas.gpu.height};
        CtrNative::DrawImage({&atlas.gpu,&sub},centered?x-l.w*scale/2:x,y,tint,scale,scale);return;
    }
}
void CtrSelectionArt::DrawStretched(const char* name,float x,float y,float width,float height,unsigned tint) {
    if(!atlas.ready)return;
    if(const auto* label=Find(name)) {const auto& l=*label;
        Tex3DS_SubTexture sub{u16(l.w),u16(l.h),float(l.x)/atlas.gpu.width,1-float(l.y)/atlas.gpu.height,
            float(l.x+l.w)/atlas.gpu.width,1-float(l.y+l.h)/atlas.gpu.height};
        CtrNative::DrawImage({&atlas.gpu,&sub},x,y,tint,width/l.w,height/l.h);return;
    }
}

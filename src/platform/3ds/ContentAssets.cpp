#include "ContentAssets.hpp"
#include "Settings.hpp"
#include "../../../assets/english_text.hpp"
#include <cstdio>
#include <cstring>
#include <string_view>
#include <unordered_map>
namespace {
bool Exists(const std::string& path) {
    FILE* file=fopen(path.c_str(),"rb");if(!file)return false;fclose(file);return true;
}
}
std::string CtrContent::Path(const char* directory,const char* name) {
    const std::string base=std::string(TH06_ASSET_ROOT)+directory+"/"+name;
    // The port's expanded display backgrounds are independent of the pack.
    // Retexture applies to game sprites/tiles, never to these fitted surfaces.
    if(std::string_view(name).ends_with(".jpg.bin") || std::string_view(name).ends_with(".bmp.bin"))return base;
    // The pack's ANM definitions are byte-identical to the originals. Cosmetic
    // choices select pixel sheets, not duplicate animation files or scripts.
    if(!strcmp(directory,"data") && std::string_view(name).ends_with(".anm"))return base;
    const bool texture=!strcmp(directory,"textures"),data=!strcmp(directory,"data");
    auto candidate=[&](const char* variant) {return std::string(TH06_ASSET_ROOT "variants/")+variant+"/"+directory+"/"+name;};
    if(!strcmp(directory,"ui") && !strcmp(name,"gameplay-material.bin") && CtrSettings::Retexture()) {
        const auto path=candidate("retexture");if(Exists(path))return path;
    }
    // Reimu and the focus indicator are independent of the graphics pack.
    if((texture || data) && (!strncmp(name,"player00.",9) || !strncmp(name,"face00",6))) {
        const auto path=candidate(CtrSettings::NewSprites()?"reimu-new":"reimu-old");
        return Exists(path)?path:base;
    }
    if((texture || data) && (!strncmp(name,"player01.",9) || !strncmp(name,"face01",6))) {
        const auto path=candidate(CtrSettings::NewSprites()?"marisa-new":"marisa-old");
        return Exists(path)?path:base;
    }
    if(texture || data || !strcmp(directory,"ui")) {
        if(CtrSettings::Retexture() && (texture || data)) {
            const auto localized=candidate(CtrSettings::English()?"retexture-en":"retexture-jp");
            if(Exists(localized))return localized;
            const auto path=candidate("retexture");if(Exists(path))return path;
        }
        if(CtrSettings::English()) {const auto path=candidate("en");if(Exists(path))return path;}
    }
    return base;
}
const char* CtrContent::Translate(const char* text) {
    if(!CtrSettings::English())return text;
    static const auto translations=[] {
        std::unordered_map<std::string_view,const char*> result;
        for(const auto& entry:CtrEnglishText::entries)result.emplace(entry.jp,entry.en);
        return result;
    }();
    const auto found=translations.find(text);if(found!=translations.end())return found->second;
    // Music titles prepend a note after formatting their Japanese name.
    if(!strncmp(text,"♪",3)) {
        const auto song=translations.find(text+3);
        if(song!=translations.end()) {static std::string title;title=std::string("♪")+song->second;return title.c_str();}
    }
    return text;
}

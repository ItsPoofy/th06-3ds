#include "PracticeSelection.hpp"
#include "Presentation.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "../../../assets/ui-font/practice_labels.hpp"
#include <cstring>
#include "SelectionArt.hpp"
#include "Settings.hpp"
#include "GameManager.hpp"
#include "MainMenu.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
namespace {
CtrAssets::PackedTexture atlas;
void Label(const char* text,float x,float y,bool selected,unsigned alpha,bool centered,bool enabled=true) {
    for(const auto& label:CtrPracticeAssets::labels)if(!std::strcmp(label.text,text)) {
        if(!atlas.ready)return;
        Tex3DS_SubTexture sub{u16(label.w),u16(label.h),float(label.x)/atlas.gpu.width,1-float(label.y)/atlas.gpu.height,
            float(label.x+label.w)/atlas.gpu.width,1-float(label.y+label.h)/atlas.gpu.height};
        if(selected && enabled) {
            CtrNative::DrawTextGradient({&atlas.gpu,&sub},centered?x-label.w*.5f:x-5,y-5,C2D_Color32(255,255,128,alpha),label.faceTop,label.faceBottom);return;
        }
        CtrNative::DrawImage({&atlas.gpu,&sub},centered?x-label.w*.5f:x-5,
            std::strstr(text,"prompt-native")?y:y-5,
            C2D_Color32(enabled?255:128,enabled?255:128,enabled?(selected?128:255):128,alpha));return;
    }
    CtrPresentation::DrawText(text,x,y,selected,alpha,centered);
}
}
bool CtrPractice::Initialize() {return atlas.ready || atlas.Load(TH06_ASSET_ROOT "ui/practice-font.bin");}
void CtrPractice::Shutdown() {atlas.Clear();}
int CtrPractice::Count() {return 6;}
int CtrPractice::UnlockedCount() {
    return std::clamp(int(g_GameManager.clrd[g_GameManager.CharacterShotType()].difficultyClearedWithoutRetries[g_GameManager.difficulty]),1,g_GameManager.difficulty==EASY?5:6);
}
bool CtrPractice::Available(int stage) {return stage>=0 && stage<UnlockedCount();}
void CtrPractice::DrawPrompt(int state,unsigned alpha,float slide) {
    const char* text=state==STATE_DIFFICULTY_SELECT?"difficulty-prompt-native":
        state==STATE_CHARACTER_SELECT?"character-prompt-native":state==STATE_SHOT_SELECT?"weapon-prompt-native":nullptr;
    if(text && alpha) {
        const auto localized=std::string(text)+(CtrSettings::English()?"-en":"");
        Label(localized.c_str(),160+slide,12,false,alpha,true);
    }
}
void CtrPractice::DrawBottom(int selected,unsigned alpha,float slide) {
    const float center=(g_GameManager.character==0?200.f:120.f)+slide;
    const float top=114.f-(18.f+24.f*(Count()-1))*.5f;
    for(int i=0;i<Count();i++) {
        char label[16];std::snprintf(label,sizeof(label),"STAGE %d",i+1);
        const bool enabled=Available(i),chosen=enabled && i==selected;
        Label(label,center,top+24*i,chosen,chosen?alpha:alpha*140/255,true,enabled);
    }
}
int CtrPractice::Hit(int x,int y) {
    const int center=g_GameManager.character==0?200:120;
    const int top=int(114.f-(18.f+24.f*(Count()-1))*.5f);
    if(x<center-70 || x>center+70 || y<top || y>=top+24*Count())return -1;
    const int row=(y-top)/24;
    return row<Count() && Available(row)?row:-1;
}

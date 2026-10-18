#include "RecordsLayout.hpp"
#include "ResultScreen.hpp"
#include "GameManager.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "TextHelper.hpp"
#include "i18n.hpp"
#include <iterator>

int CtrRecords::PageCount() {return (std::size(g_GameManager.catk)+PageSize-1)/PageSize;}
void CtrRecords::PrepareSpellPage(ResultScreen& screen) {
    for(int row=0;row<PageSize;row++) {
        const int index=screen.lastSpellcardSelected*PageSize+row;
        if(index>=int(std::size(g_GameManager.catk)))break;
        const auto& card=g_GameManager.catk[index];
        auto& vm=screen.unk_28a0[row];vm.fontWidth=vm.fontHeight=15;
        TextHelper::Wrapped(&vm,card.numAttempts?card.name:TH_UNKNOWN_SPELLCARD,COLOR_RGB(COLOR_WHITE),COLOR_RGB(COLOR_BLACK),286);
    }
}
void CtrRecords::DrawSpellPage(ResultScreen& screen,const ZunVec3& origin) {
    if(screen.lastSpellcardSelected<0)return;
    constexpr float scale=1.2f;
    const auto savedScale=g_AsciiManager.scale;const auto savedColor=g_AsciiManager.color;
    g_AsciiManager.scale=ZunVec2(scale,scale);
    for(int row=0;row<PageSize;row++) {
        const int index=screen.lastSpellcardSelected*PageSize+row;
        if(index>=int(std::size(g_GameManager.catk)))break;
        const auto& card=g_GameManager.catk[index];
        const auto color=card.numAttempts==0?0x80c0c0ffu:card.numSuccess==0?0xffc0a0a0u:0xfff0f0ffu;
        ZunVec3 position(origin.x,origin.y+16+60*row,origin.z);
        g_AsciiManager.color=color;
        g_AsciiManager.AddFormatText(&position,"No.%.2d",index+1);
        auto vm=screen.unk_28a0[row];if(!vm.sprite)continue;auto sprite=*vm.sprite;
        sprite.uvStart.x+=vm.ctrTextStart/sprite.textureWidth;
        sprite.widthPx=vm.ctrTextWidth;
        sprite.heightPx=vm.ctrTextHeight;
        sprite.uvEnd.x=sprite.uvStart.x+sprite.widthPx/sprite.textureWidth;
        sprite.uvEnd.y=(sprite.startPixelInclusive.y+sprite.heightPx-.5f)/sprite.textureHeight;
        vm.sprite=&sprite;vm.flags.anchor=AnmVmAnchor_TopLeft;
        vm.scaleX=vm.scaleY=scale;vm.posOffset=ZunVec3(0,0,0);
        vm.pos=ZunVec3(origin.x+90,position.y,origin.z);vm.color=color;
        g_AnmManager->DrawNoRotation(&vm);g_AnmManager->SetCurrentSprite(nullptr);
        position.x=origin.x+464;
        g_AsciiManager.AddFormatText(&position,"%3d/%3d",card.numSuccess,card.numAttempts);
    }
    g_AsciiManager.scale=savedScale;g_AsciiManager.color=savedColor;
}

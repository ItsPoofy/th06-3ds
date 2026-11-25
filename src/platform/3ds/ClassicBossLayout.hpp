#pragma once
#include <algorithm>

namespace CtrClassicBossLayout {
inline constexpr float BarLeft=88,BarTop=22,BarHeight=6,TimerGap=12;
inline constexpr float NameLeft=32,NameTop=34,NameScale=.65f,TimerTop=16;
inline constexpr float BossRight=416,PlayerLeft=56,PlayerBottom=436;
inline constexpr float TitleTop=34,TitleBandWidth=255,TitleGap=6;
inline constexpr float InfoScale=.5f,BonusLeft=180,BonusRight=304,HistoryLeft=312,HistoryRight=416;
// Ink widths in the port's boss-spell-hud atlas, before scaling.
inline constexpr float NameInk[11]={173,193,78,115,68,154,110,247,164,186,187};
inline float TitleLeft(int name) {
    return std::max(BossRight-TitleBandWidth,NameLeft+NameInk[std::clamp(name,0,10)]*NameScale+TitleGap);
}
inline float TimerLeft(float scale) {return BossRight-30*scale;}
inline float BarWidth(float timerScale) {return TimerLeft(timerScale)-TimerGap-BarLeft;}
inline float MarkerLeft(int index,int count) {return 35+index*45.f/count;}
inline float MarkerRight(int index,int count) {return 35+(index+1)*45.f/count-(count<=5?3:2);}
inline float SpellFraction(bool active,int threshold,int maxLife) {
    return active?1.f:std::clamp(float(threshold)/std::max(1,maxLife),0.f,1.f);
}
}

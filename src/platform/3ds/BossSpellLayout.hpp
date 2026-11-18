#pragma once
#include <algorithm>

namespace CtrBossSpellLayout {
// TH20 ascii.anm: sprite 285 is 510x78 at twice logical resolution;
// The non-spell timer sits at the playfield's top edge. Spell timers retain
// TH20's lower destination, with the same animated transition.
inline constexpr float Right=416, Center=224, BandWidth=255, BandHeight=39;
// The boss name is drawn at NameLeft at half size from its 256 px cell in
// boss-spell-hud (tools/converter/boss_hud.py). A spell card title may extend
// left until TitleGap short of the name's ink, measured here per name.
inline constexpr float NameLeft=36,NameScale=.5f,TitleGap=4;
inline constexpr float NameInk[11]={173,193,78,115,68,154,110,247,164,186,187};
inline float TitleLeft(int name) {return NameLeft+NameInk[std::clamp(name,0,10)]*NameScale+TitleGap;}
inline constexpr float TimerCenter=16+8, SpellTimerCenter=16+51;
inline constexpr float InfoScale=.5f;
inline constexpr float LabelScale=InfoScale,DigitScale=InfoScale,LabelDrop=0;
// The information row sits at the right edge as in TH20: each label, then its
// right-aligned counter. Each field fits its label's ink (Bonus 78, History 87
// atlas px) beside its widest counter, 7 digits or "00/00", with a small gap.
inline constexpr float BonusLeft=206, BonusRight=316;
inline constexpr float HistoryLeft=322, HistoryRight=416;
inline float Baseline(float titleHeight) {return 16+titleHeight+1;}
inline float ExtraHeight(float titleHeight) {return std::max(0.f,titleHeight-20);}
inline float InfoTop(float titleHeight) {return Baseline(titleHeight)+2;}
inline float TimerDestination(float titleHeight) {return SpellTimerCenter-TimerCenter+ExtraHeight(titleHeight);}
}

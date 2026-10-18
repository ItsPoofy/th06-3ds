#pragma once
#include <algorithm>

namespace CtrBossSpellLayout {
// TH20 ascii.anm: sprite 285 is 510x78 at twice logical resolution;
// The non-spell timer sits at the playfield's top edge. Spell timers retain
// TH20's lower destination, with the same animated transition.
inline constexpr float Right=416, Center=224, BandWidth=255, BandHeight=39;
inline constexpr float TimerCenter=16+8, SpellTimerCenter=16+51;
inline constexpr float InfoScale=.5f;
// The information row occupies the space to the timer's right. Fixed field
// edges preserve TH20's alignment with the port's wider DFPOP label glyphs.
inline constexpr float BonusLeft=216, BonusRight=328;
inline constexpr float HistoryLeft=332, HistoryRight=416;
inline float Baseline(float titleHeight) {return 16+titleHeight+1;}
inline float ExtraHeight(float titleHeight) {return std::max(0.f,titleHeight-20);}
inline float InfoTop(float titleHeight) {return Baseline(titleHeight)+2;}
inline float TimerDestination(float titleHeight) {return SpellTimerCenter-TimerCenter+ExtraHeight(titleHeight);}
}

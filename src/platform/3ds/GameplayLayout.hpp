#pragma once
#include "Settings.hpp"
#include "DisplaySettings.hpp"
namespace CtrLayout {
inline constexpr float NameCardHeight=64.f;
inline bool Single() {return CtrSettings::Mode()==CtrSettings::ScreenMode::TopOnly;}
inline float Scale() {return Single()?240.f/448.f:5.f/6.f;}
inline float Left(bool top=true) {return ((top?400.f:320.f)-384.f*Scale())*.5f;}
inline float Offset() {return Single()?0.f:CtrDisplay::PlayfieldOffset();}
}

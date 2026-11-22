#pragma once
#include "Supervisor.hpp"

namespace CtrMenuCanvas {
inline constexpr int Width=800;
// Preserve the original 40 screen pixels around the 640 px scene's content.
inline constexpr int ContentInset=80;
inline bool FullWidth() {
    return g_Supervisor.wantedState==SUPERVISOR_STATE_MUSICROOM;
}
inline float ScreenLeft(){return FullWidth()?0.f:40.f;}
}

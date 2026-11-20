#pragma once
#include "Supervisor.hpp"

namespace CtrMenuCanvas {
inline constexpr int Width=800;
inline bool FullWidth() {
    const int scene=g_Supervisor.wantedState;
    return scene==SUPERVISOR_STATE_MUSICROOM || scene==SUPERVISOR_STATE_RESULTSCREEN ||
        scene==SUPERVISOR_STATE_RESULTSCREEN_FROMGAME;
}
inline float ScreenLeft(){return FullWidth()?0.f:40.f;}
}

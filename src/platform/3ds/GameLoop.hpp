#pragma once
#include "GameWindow.hpp"
namespace CtrGameLoop {
    void ResetClock();
    void RequestClockReset();
    RenderResult Render(GameWindow& window);
}

#pragma once
#include "ZunColor.hpp"
class CtrRenderer;
namespace CtrBossSpellHud {
bool Initialize(CtrRenderer& renderer);
void Shutdown(CtrRenderer& renderer);
void Reset();
void Begin();
void End();
void Tick();
bool DrawTimer(int fallbackSeconds,ZunColor color);
}

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
// Draws the Bonus and History row. Call after the spell card name, whose
// backdrop would otherwise cover the row.
void DrawInfo();
}

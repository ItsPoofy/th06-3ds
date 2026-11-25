#pragma once
#include "ZunColor.hpp"
struct AnmVm;
class CtrRenderer;
namespace CtrClassicBossHud {
void BeginStage(CtrRenderer& renderer);
void Shutdown(CtrRenderer& renderer);
void BeginSpell();
void EndSpell();
void Tick();
void DrawBar();
void DrawTimer(int seconds,ZunColor color);
void DrawSpellcard(const AnmVm& source);
void DrawInfo();
}

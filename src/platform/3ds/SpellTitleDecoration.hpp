#pragma once
#include "ZunColor.hpp"
class CtrRenderer;
namespace CtrSpellTitleDecoration {
void Initialize(CtrRenderer& renderer);
void Shutdown(CtrRenderer& renderer);
void Underline(float left,float right,float top,bool player,ZunColor color);
void BossButterflies(float right,float baseline,float zoom,ZunColor color);
}

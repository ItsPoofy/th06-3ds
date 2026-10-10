#pragma once
class CtrRenderer;
namespace CtrBossHealthBar {
bool Available();
bool Initialize(CtrRenderer& renderer);
void Shutdown(CtrRenderer& renderer);
void Reset();
void Tick();
void Draw();
}

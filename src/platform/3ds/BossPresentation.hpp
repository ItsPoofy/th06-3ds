#pragma once
struct Enemy;
struct Effect;
namespace CtrBossPresentation {
void Reset();
void Begin(const Enemy* actor);
void TrackDefeat(const Enemy* actor,const Effect* effect);
void Retire(const Enemy* actor);
bool Visible();
}

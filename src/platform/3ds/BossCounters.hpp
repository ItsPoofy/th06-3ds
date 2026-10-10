#pragma once
#include <cstddef>
namespace CtrBossCounters {
void Load(const void* ecl,std::size_t size,int difficulty,int stage=0);
void Begin(const void* actor,int sub);
void SetLives(const void* actor,int lives,int sub=-1);
void SetCallback(const void* actor,int sub);
void StartSpell(const void* actor,int sub);
void EndSpell(const void* actor);
void Observe(bool present);
int Lives();
int Spells();
}

#pragma once
#include "ZunResult.hpp"
struct Enemy;
struct EclRawInstr;
namespace Practice::Th06Run {
enum class SetupStep { Execute, Skip, Stop };
bool Begin();
void End();
void Tick();
void Spawned(Enemy* enemy,int sub);
bool AttackEnded(Enemy* enemy,bool spell);
bool Preparing();
SetupStep BeforeInstruction(Enemy* enemy,EclRawInstr* instruction);
int ShotRoute();
bool QedRage();
}

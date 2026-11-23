#pragma once
#include "ZunResult.hpp"
#include "Completion.hpp"
struct Enemy;
struct EclRawInstr;
namespace Practice::Th06Run {
enum class SetupStep { Execute, Skip, Stop };
bool Begin();
void End();
void Tick();
void BeforeTimeline();
void Spawned(Enemy* enemy,int sub);
bool AttackEnded(Enemy* enemy,bool spell);
void StageEnded();
bool PlayerDied();
bool Leave(Completion::Action action);
bool ContinueOutro(Enemy* enemy,const EclRawInstr* instruction);
bool Preparing();
SetupStep BeforeInstruction(Enemy* enemy,EclRawInstr* instruction);
int ShotRoute();
bool QedRage();
}

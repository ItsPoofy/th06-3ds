#pragma once
#include <citro2d.h>
struct GuiMsgVm;
struct AnmVm;
namespace CtrDecorations {
void BeginStage(int stage);
int CurrentBossName();
void UpdateBoss(bool present,int portrait);
void UpdateDialogue(const GuiMsgVm& message);
void BeginDialogue();
void DialogueSpeaker(int side);
void PortraitInterrupt(int side,int interrupt);
void EndDialogue();
void SelectPortrait(int portrait);
// A practice start at a boss skips the dialogue that names it. Midboss starts
// show the stage's first boss face; end boss starts its main boss.
void PracticeBoss(bool midboss);
void PlayerSpellcard(int sprite);
void EndPlayerSpellcard();
void BossSpellcard(int sprite);
void EndBossSpellcard();
void DrawPortrait(C3D_RenderTarget* top);
void DrawHitbox(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
void Shutdown();
}

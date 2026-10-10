#pragma once
#include <citro2d.h>
struct GuiMsgVm;
struct AnmVm;
namespace CtrDecorations {
void BeginStage(int stage);
void UpdateBoss(bool present,int portrait);
void UpdateDialogue(const GuiMsgVm& message);
void BeginDialogue();
void PortraitInterrupt(int side,int interrupt);
void EndDialogue();
void SelectPortrait(int portrait);
void PlayerSpellcard(int sprite);
void EndPlayerSpellcard();
void BossSpellcard(int sprite);
void EndBossSpellcard();
void DrawPortrait(C3D_RenderTarget* top);
void DrawHitbox(C3D_RenderTarget* top,C3D_RenderTarget* bottom);
void Shutdown();
}

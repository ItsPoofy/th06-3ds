#pragma once

namespace Practice::RunTransition {
inline constexpr int FadeFrames=12;
void Reset();
void BeginEntry();
void BeginExit();
// Advance after presenting the overlay, so simulation catch-up cannot skip a fade.
void Presented();
float Opacity();
bool Active();
bool ExitPresented();
}

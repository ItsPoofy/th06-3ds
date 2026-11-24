#pragma once

namespace Practice::Retry {
// A same-stage retry keeps loaded assets through teardown, then rebuilds runtime state.
void Begin();
void Restore();
void End();
bool Releasing();
bool Restoring();
}

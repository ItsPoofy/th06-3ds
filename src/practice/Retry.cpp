#include "Retry.hpp"
#include "AnmManager.hpp"

namespace {
enum class Phase { Idle, Release, Restore };
Phase phase=Phase::Idle;
}
void Practice::Retry::Begin() {
    g_AnmManager->FlushVertexBuffer();
    phase=Phase::Release;
}
void Practice::Retry::Restore() {if(phase==Phase::Release)phase=Phase::Restore;}
void Practice::Retry::End() {
    if(phase!=Phase::Idle) {
        g_AnmManager->FlushVertexBuffer();
        g_AnmManager->SetCurrentSprite(nullptr);
        g_AnmManager->SetCurrentTexture({});
    }
    phase=Phase::Idle;
}
bool Practice::Retry::Releasing() {return phase==Phase::Release;}
bool Practice::Retry::Restoring() {return phase==Phase::Restore;}

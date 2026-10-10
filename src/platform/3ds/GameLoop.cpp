#include "GameLoop.hpp"
#include "FixedStepClock.hpp"
#include "CtrRenderer.hpp"
#include "Presentation.hpp"
#include "AnmManager.hpp"
#include "Chain.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include <SDL2/SDL_timer.h>
#include <atomic>

namespace {
FixedStepClock simulationClock;
unsigned clockGeneration=0;
std::atomic_bool resetRequested=false;

void DrawFrame() {
    // curState is the requested scene; wantedState is the installed chain.
    // A retiring scene can already have removed its draw callback. Retain the
    // completed display until Supervisor constructs and draws the next one.
    if(g_Supervisor.curState!=g_Supervisor.wantedState) return;
    CtrPresentation::BeginScene();
    ZunViewport fullscreen{0,0,GAME_WINDOW_WIDTH,GAME_WINDOW_HEIGHT,0,1};
    fullscreen.Set();
    // Main-menu backgrounds are composed once at the native 400px width.
    // Its engine layer contains only the original animated foreground sprites.
    const bool playing=g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER ||
        g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER_REINIT;
    const auto color=playing?(g_Stage.skyFog.color|0xff000000u):0u;
    g_GfxBackend->SetClearColor(((color>>16)&255)/255.0f,((color>>8)&255)/255.0f,
        (color&255)/255.0f,(color>>24)/255.0f);
    g_GfxBackend->Clear(CLEAR_COLOR_BUFFER|CLEAR_DEPTH_BUFFER);
    g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
    g_Supervisor.viewport.Set();
    g_AnmManager->ClearVertexBuffer();
    g_AnmManager->flushesThisFrame=0;
    static_cast<CtrRenderer*>(g_GfxBackend)->SetGameplayLayout(
        g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER ||
        g_Supervisor.wantedState==SUPERVISOR_STATE_GAMEMANAGER_REINIT);
    g_Chain.RunDrawChain();
    g_AnmManager->SetCurrentTexture(0);
    g_AnmManager->FlushVertexBuffer();
    if(CtrPresentation::HasScene()) static_cast<CtrRenderer*>(g_GfxBackend)->CompleteScene();
}
}

void CtrGameLoop::ResetClock() {
    simulationClock.Reset(SDL_GetPerformanceCounter(),SDL_GetPerformanceFrequency());
    ++clockGeneration;
}
void CtrGameLoop::RequestClockReset() { resetRequested=true; }

RenderResult CtrGameLoop::Render(GameWindow& window) {
    if(resetRequested.exchange(false)) ResetClock();
    if(!window.lastActiveAppValue) {ResetClock();return RENDER_RESULT_KEEP_RUNNING;}
    if(!clockGeneration) ResetClock();
    simulationClock.Advance(SDL_GetPerformanceCounter());
    // Calculate before drawing so fresh input reaches the current display.
    // The fixed 60 Hz clock still controls simulation and replay timing.
    const unsigned generation=clockGeneration;
    for(unsigned steps=0;steps<8 && simulationClock.ConsumeStep();++steps) {
        g_Supervisor.framerateMultiplier=1.0f;
        g_Supervisor.effectiveFramerateMultiplier=1.0f;
        g_Supervisor.viewport.x=0;g_Supervisor.viewport.y=0;
        g_Supervisor.viewport.width=GAME_WINDOW_WIDTH;
        g_Supervisor.viewport.height=GAME_WINDOW_HEIGHT;
        g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
        g_Supervisor.viewport.Set();
        const int result=g_Chain.RunCalcChain();
        g_SoundPlayer.PlaySounds();
        if(result==0) return RENDER_RESULT_EXIT_SUCCESS;
        if(result==-1) return RENDER_RESULT_EXIT_ERROR;
        // Loading establishes a new scene and clock epoch. Do not run old
        // queued updates against the scene just constructed.
        // The new scene has not been drawn yet. Keep the previous completed
        // display intact until the next render.
        if(generation!=clockGeneration) return RENDER_RESULT_KEEP_RUNNING;
    }
    // Unconsumed steps remain queued if a frame needs more than eight updates.
    // Never silently discard elapsed gameplay time during normal operation.
    DrawFrame();
    window.curFrame=0;
    GameWindow::Present();
    return RENDER_RESULT_KEEP_RUNNING;
}

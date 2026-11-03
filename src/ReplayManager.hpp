#pragma once

#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "ReplayData.hpp"
#include "inttypes.hpp"

struct ReplayManager
{
    static ZunResult RegisterChain(i32 isDemo, const char *replayFile);
    static ChainCallbackResult OnUpdate(ReplayManager *mgr);
    static ChainCallbackResult OnUpdateDemoHighPrio(ReplayManager *mgr);
    static ChainCallbackResult OnUpdateDemoLowPrio(ReplayManager *mgr);
    static ChainCallbackResult OnDraw(ReplayManager *mgr);
    static ZunResult AddedCallback(ReplayManager *mgr);
    static ZunResult AddedCallbackDemo(ReplayManager *mgr);
    static ZunResult DeletedCallback(ReplayManager *mgr);
    static void StopRecording();
    static bool SaveReplay(const char *replay_path, char *param_2);
    static ZunResult ValidateReplayData(const ReplayHeader *data, i32 fileSize);

    ReplayManager()
    {
    }

    i32 IsDemo() const
    {
        return this->isDemo;
    }

    i32 frameId=0;
    ReplayData *replayData=nullptr;
    i32 isDemo=0;
    const char *replayFile=nullptr;
    u8 unk10[52]{};
    u16 unk44=0;
    ReplayDataInput *replayInputs=nullptr;
    const ReplayDataInput *replayInputStageBookmarks[7]{};
    ChainElem *calcChain=nullptr;
    ChainElem *drawChain=nullptr;
    ChainElem *calcChainDemoHighPrio=nullptr;
    bool recordingStopped=false,recordingOverflow=false;
};
extern ReplayManager* g_ReplayManager;

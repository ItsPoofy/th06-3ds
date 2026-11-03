#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>
#include "ReplayStorage.hpp"

#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ReplayManager.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"
#include "utils.hpp"
#ifdef __3DS__
#include "platform/3ds/Settings.hpp"
#endif

ReplayManager *g_ReplayManager;

ZunResult ReplayManager::ValidateReplayData(const ReplayHeader *data, i32 fileSize)
{
    const u8 *checksumCursor;
    u32 checksum;
    u8 *obfuscateCursor;
    u8 obfOffset;
    i32 idx;

    if (data == NULL || fileSize<i32(sizeof(ReplayHeader)))
    {
        return ZUN_ERROR;
    }

    /* "T6RP" magic bytes */
    if (*(i32 *)data->magic != *(i32 *)"T6RP")
    {
        return ZUN_ERROR;
    }

    /* Deobfuscate the replay decryptedData */
    obfuscateCursor = reinterpret_cast<u8*>(const_cast<ReplayHeader*>(data))+offsetof(ReplayHeader,rngValue3);
    obfOffset = data->key;
    for (idx = 0; idx < fileSize - (i32)offsetof(ReplayHeader, rngValue3); idx += 1, obfuscateCursor += 1)
    {
        *obfuscateCursor -= obfOffset;
        obfOffset += 7;
    }

    /* Calculate the checksum */
    /* (0x3f000318 + key + sum(c for c in decryptedData)) % (2 ** 32) */
    checksumCursor = reinterpret_cast<const u8*>(data)+offsetof(ReplayHeader,key);
    checksum = 0x3f000318;
    for (idx = 0; idx < fileSize - (i32)offsetof(ReplayHeader, key); idx += 1, checksumCursor += 1)
    {
        checksum += *checksumCursor;
    }

    if (checksum != data->checksum)
    {
        return ZUN_ERROR;
    }

    if (data->version != GAME_VERSION)
    {
        return ZUN_ERROR;
    }
    if(data->shottypeChara>=4 || data->difficulty>4)return ZUN_ERROR;
    u32 previous=sizeof(ReplayHeader);
    for(int stage=0;stage<7;stage++) {
        const u32 start=data->stageReplayDataOffsets[stage];
        if(!start)continue;
        u32 end=fileSize;
        for(int next=stage+1;next<7;next++)if(data->stageReplayDataOffsets[next]) {
            end=data->stageReplayDataOffsets[next];break;
        }
        constexpr auto prefix=offsetof(StageReplayData,replayInputs);
        if(start<previous || start%4 || end>u32(fileSize) || end<start || end-start<prefix+16 ||
           (end-start-prefix)%sizeof(ReplayDataInput)!=0)return ZUN_ERROR;
        const auto count=(end-start-prefix)/sizeof(ReplayDataInput);
        if(count>53998)return ZUN_ERROR;
        const auto* inputs=reinterpret_cast<const StageReplayData*>(reinterpret_cast<const u8*>(data)+start)->replayInputs;
        bool terminated=false;
        for(size_t i=0;i<count;i++) {
            if(inputs[i].frameNum<0 || (i && inputs[i].frameNum<inputs[i-1].frameNum))return ZUN_ERROR;
            // Playback stops consuming events at the first end marker. Native
            // files (including demo00.rpy) can retain an older stop sequence
            // after it; those unused records are not part of the input stream.
            if(inputs[i].frameNum==9999999) {terminated=i>0;break;}
        }
        if(!terminated)return ZUN_ERROR;
        previous=end;
    }

    return ZUN_SUCCESS;
}

ZunResult ReplayManager::RegisterChain(i32 isDemo, const char *replayFile)
{
    ReplayManager *replayMgr;

    if (g_Supervisor.framerateMultiplier < 0.99f && !isDemo)
    {
        return ZUN_SUCCESS;
    }
    g_Supervisor.framerateMultiplier = 1.0f;
    if (g_ReplayManager == NULL)
    {
        replayMgr = new ReplayManager();
        g_ReplayManager = replayMgr;
        replayMgr->replayData = NULL;
        replayMgr->isDemo = isDemo;
        replayMgr->replayFile = replayFile;
        switch (isDemo)
        {
        case false:
            replayMgr->calcChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdate);
            replayMgr->calcChain->addedCallback = (ChainAddedCallback)AddedCallback;
            replayMgr->calcChain->deletedCallback = (ChainDeletedCallback)DeletedCallback;
            replayMgr->drawChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnDraw);
            replayMgr->calcChain->arg = replayMgr;
            if (g_Chain.AddToCalcChain(replayMgr->calcChain, TH_CHAIN_PRIO_CALC_REPLAYMANAGER))
            {
                delete replayMgr->drawChain;
                replayMgr->drawChain = NULL;
                g_Chain.Cut(replayMgr->calcChain);
                return ZUN_ERROR;
            }
            replayMgr->calcChainDemoHighPrio = NULL;
            break;
        case true:
            replayMgr->calcChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdateDemoHighPrio);
            replayMgr->calcChain->addedCallback = (ChainAddedCallback)AddedCallbackDemo;
            replayMgr->calcChain->deletedCallback = (ChainDeletedCallback)DeletedCallback;
            replayMgr->drawChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnDraw);
            replayMgr->calcChain->arg = replayMgr;
            if (g_Chain.AddToCalcChain(replayMgr->calcChain, TH_CHAIN_PRIO_CALC_LOW_PRIO_REPLAYMANAGER_DEMO))
            {
                delete replayMgr->drawChain;
                replayMgr->drawChain = NULL;
                g_Chain.Cut(replayMgr->calcChain);
                return ZUN_ERROR;
            }
            replayMgr->calcChainDemoHighPrio = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdateDemoLowPrio);
            replayMgr->calcChainDemoHighPrio->arg = replayMgr;
            g_Chain.AddToCalcChain(replayMgr->calcChainDemoHighPrio, TH_CHAIN_PRIO_CALC_HIGH_PRIO_REPLAYMANAGER_DEMO);
            break;
        }
        replayMgr->drawChain->arg = replayMgr;
        g_Chain.AddToDrawChain(replayMgr->drawChain, TH_CHAIN_PRIO_DRAW_REPLAYMANAGER);
    }
    else
    {
        switch (isDemo)
        {
        case false:
            AddedCallback(g_ReplayManager);
            break;
        case true:
            return AddedCallbackDemo(g_ReplayManager);
            break;
        }
    }
    return ZUN_SUCCESS;
}

#ifdef __3DS__
// ENTER carries a physical dialogue-confirm pulse independently of latched
// fire. The existing 16-bit replay input field already accommodates it.
#define TH_BUTTON_REPLAY_CAPTURE \
    (TH_BUTTON_SHOOT | TH_BUTTON_BOMB | TH_BUTTON_FOCUS | TH_BUTTON_SKIP | TH_BUTTON_DIRECTION | TH_BUTTON_ENTER)
#else
#define TH_BUTTON_REPLAY_CAPTURE                                                                                       \
    (TH_BUTTON_SHOOT | TH_BUTTON_BOMB | TH_BUTTON_FOCUS | TH_BUTTON_SKIP | TH_BUTTON_DIRECTION)
#endif

ChainCallbackResult ReplayManager::OnUpdate(ReplayManager *mgr)
{
    u16 inputs;

    if (!g_GameManager.isInMenu || mgr->recordingStopped)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    inputs = IS_PRESSED(TH_BUTTON_REPLAY_CAPTURE);
    if (inputs != mgr->replayInputs->inputKey)
    {
        const auto* start=mgr->replayData->stageReplayData[g_GameManager.currentStage-1]->replayInputs;
        if(mgr->replayInputs-start>=53995) {
            mgr->recordingOverflow=true;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        mgr->replayInputs += 1;
        mgr->replayInputStageBookmarks[g_GameManager.currentStage - 1] = mgr->replayInputs + 1;
        mgr->replayInputs->frameNum = mgr->frameId;
        mgr->replayInputs->inputKey = inputs;
    }
    mgr->frameId += 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnUpdateDemoLowPrio(ReplayManager *mgr)
{
    if (!g_GameManager.isInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (g_Gui.HasCurrentMsgIdx() && g_Gui.IsDialogueSkippable() && mgr->frameId % 3 != 2)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnUpdateDemoHighPrio(ReplayManager *mgr)
{
    if (!g_GameManager.isInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    while (mgr->frameId >= mgr->replayInputs[1].frameNum)
    {
        mgr->replayInputs += 1;
    }
    g_CurFrameInput = IS_PRESSED(0xFFFFFFFF & ~TH_BUTTON_REPLAY_CAPTURE) | mgr->replayInputs->inputKey;
    g_IsEigthFrameOfHeldInput = 0;
    if (g_LastFrameInput == g_CurFrameInput)
    {
        if (30 <= g_NumOfFramesInputsWereHeld)
        {
            if (g_NumOfFramesInputsWereHeld % 8 == 0)
            {
                g_IsEigthFrameOfHeldInput = 1;
            }
            if (38 <= g_NumOfFramesInputsWereHeld)
            {
                g_NumOfFramesInputsWereHeld = 30;
            }
        }
        g_NumOfFramesInputsWereHeld++;
    }
    else
    {
        g_NumOfFramesInputsWereHeld = 0;
    }
    mgr->frameId += 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnDraw(ReplayManager *mgr)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

inline StageReplayData *AllocateStageReplayData(i32 size)
{
    return (StageReplayData *)std::malloc(size);
}

inline void ReleaseReplayData(void *data)
{
    return std::free(data);
}

inline void ReleaseStageReplayData(void *data)
{
    return std::free(data);
}

ZunResult ReplayManager::AddedCallback(ReplayManager *mgr)
{
    StageReplayData *stageReplayData;
    StageReplayData *oldStageReplayData;
    i32 idx;

    mgr->frameId = 0;
    mgr->recordingStopped=false;
    if (mgr->replayData == NULL)
    {
        mgr->replayData = new ReplayData();
        mgr->replayData->header = new ReplayHeader();
        std::memcpy(&mgr->replayData->header->magic[0], "T6RP", 4);
        mgr->replayData->header->shottypeChara = g_GameManager.character * 2 + g_GameManager.shotType;
        mgr->replayData->header->version = 0x102;
        mgr->replayData->header->difficulty = g_GameManager.difficulty;
        std::memcpy(&mgr->replayData->header->name, "NO NAME", 4);
        for (idx = 0; idx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); idx += 1)
        {
            mgr->replayData->stageReplayData[idx] = NULL;
        }
    }
    else
    {
        oldStageReplayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 2];
        if (oldStageReplayData == NULL)
        {
            return ZUN_ERROR;
        }
        oldStageReplayData->score = g_GameManager.score;
    }
    if (mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] != NULL)
    {
        utils::DebugPrint2("error : replay.cpp");
    }
    mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] = static_cast<StageReplayData*>(std::calloc(1,sizeof(StageReplayData)));
    stageReplayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 1];
    if(!stageReplayData)return ZUN_ERROR;
    stageReplayData->bombsRemaining = g_GameManager.bombsRemaining;
    stageReplayData->livesRemaining = g_GameManager.livesRemaining;
    stageReplayData->power = g_GameManager.currentPower;
    stageReplayData->rank = g_GameManager.rank;
    stageReplayData->pointItemsCollected = g_GameManager.pointItemsCollected;
    stageReplayData->randomSeed = g_GameManager.randomSeed;
    stageReplayData->powerItemCountForScore = g_GameManager.powerItemCountForScore;
#ifdef __3DS__
    // Preserve the collection rule per stage without changing the replay layout.
    CtrSettings::EndReplayCollection();
    // Language changes dialogue lengths/timing. Player and graphics variants
    // can also select animation scripts. Replay with the recorded content.
    stageReplayData->padding[0]='R';stageReplayData->padding[1]=1;
    stageReplayData->padding[2]=(CtrSettings::Enabled(CtrSettings::Toggle::AutoCollect)?1:0)|
        (CtrSettings::English()?2:0)|(CtrSettings::Retexture()?4:0)|(CtrSettings::NewSprites()?8:0);
#endif
    mgr->replayInputs = stageReplayData->replayInputs;
    mgr->replayInputs->frameNum = 0;
    mgr->replayInputs->inputKey = 0;
    mgr->replayInputStageBookmarks[g_GameManager.currentStage-1]=mgr->replayInputs+1;
    mgr->unk44 = 0;
    return ZUN_SUCCESS;
}

ZunResult ReplayManager::AddedCallbackDemo(ReplayManager *mgr)
{
    i32 idx;
    StageReplayData *replayData;

    mgr->frameId = 0;
    if (mgr->replayData == NULL)
    {
        mgr->replayData = (ReplayData *)std::malloc(sizeof(ReplayData));

        mgr->replayData->header = (ReplayHeader *)FileSystem::OpenPath(mgr->replayFile, g_GameManager.demoMode == 0);
        if (ValidateReplayData(mgr->replayData->header, g_LastFileSize) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        for (idx = 0; idx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); idx += 1)
        {
            if (mgr->replayData->header->stageReplayDataOffsets[idx] != 0)
            {
                mgr->replayData->stageReplayData[idx] =
                    (StageReplayData *)(((u8 *)mgr->replayData->header) +
                                        mgr->replayData->header->stageReplayDataOffsets[idx]);
            }
            else
            {
                mgr->replayData->stageReplayData[idx] = NULL;
            }
        }
    }
    if (mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] == NULL)
    {
        return ZUN_ERROR;
    }
    replayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 1];
#ifdef __3DS__
    // Native/older replays use the original max-power collection rule.
    if(replayData->padding[0]=='R' && replayData->padding[1]==1)
        CtrSettings::ReplayOptions(static_cast<unsigned char>(replayData->padding[2]));
    else CtrSettings::ReplayCollection(std::memcmp(replayData->padding,"POC",3)==0);
#endif
    g_GameManager.character = mgr->replayData->header->shottypeChara / 2;
    g_GameManager.shotType = mgr->replayData->header->shottypeChara % 2;
    g_GameManager.difficulty = (Difficulty)mgr->replayData->header->difficulty;
    g_GameManager.pointItemsCollected = replayData->pointItemsCollected;
    g_Rng.Initialize(replayData->randomSeed);
    g_GameManager.rank = replayData->rank;
    g_GameManager.livesRemaining = replayData->livesRemaining;
    g_GameManager.bombsRemaining = replayData->bombsRemaining;
    g_GameManager.currentPower = replayData->power;
    mgr->replayInputs = replayData->replayInputs;
    g_GameManager.powerItemCountForScore = replayData->powerItemCountForScore;
    if (2 <= g_GameManager.currentStage && mgr->replayData->stageReplayData[g_GameManager.currentStage - 2] != NULL)
    {
        g_GameManager.guiScore = mgr->replayData->stageReplayData[g_GameManager.currentStage - 2]->score;
        g_GameManager.score = g_GameManager.guiScore;
    }
    return ZUN_SUCCESS;
}

ZunResult ReplayManager::DeletedCallback(ReplayManager *mgr)
{
#ifdef __3DS__
    CtrSettings::EndReplayCollection();
#endif
    g_Chain.Cut(mgr->drawChain);
    mgr->drawChain = NULL;
    if (mgr->calcChainDemoHighPrio != NULL)
    {
        g_Chain.Cut(mgr->calcChainDemoHighPrio);
        mgr->calcChainDemoHighPrio = NULL;
    }
    std::free(g_ReplayManager->replayData->header);
    ReleaseReplayData(g_ReplayManager->replayData);
    delete g_ReplayManager;
    g_ReplayManager = NULL;
    g_ReplayManager = NULL;
    return ZUN_SUCCESS;
}

void ReplayManager::StopRecording()
{
    ReplayManager *mgr = g_ReplayManager;
    if (mgr != NULL && !mgr->IsDemo() && !mgr->recordingStopped)
    {
        mgr->replayInputs += 1;
        mgr->replayInputs->frameNum = mgr->frameId;
        mgr->replayInputs->inputKey = 0;
        mgr->replayInputs += 1;
        mgr->replayInputs->frameNum = 9999999;
        mgr->replayInputs->inputKey = 0;
        mgr->replayInputStageBookmarks[g_GameManager.currentStage - 1] = mgr->replayInputs + 1;
        mgr->recordingStopped=true;
    }
}

bool ReplayManager::SaveReplay(const char* replayPath,char* replayName)
{
    auto* mgr=g_ReplayManager;
    if(!mgr)return true;
    if(!mgr->IsDemo() && replayPath) {
        if(mgr->recordingOverflow)return false;
        StopRecording();
        ReplayHeader header=*mgr->replayData->header;
        std::vector<u8> bytes(sizeof(header));
        for(int stage=0;stage<7;stage++) {
            const auto* data=mgr->replayData->stageReplayData[stage];
            header.stageReplayDataOffsets[stage]=0;
            if(!data)continue;
            const auto* end=mgr->replayInputStageBookmarks[stage];
            if(!end)return false;
            header.stageReplayDataOffsets[stage]=bytes.size();
            const auto* begin=reinterpret_cast<const u8*>(data);
            bytes.insert(bytes.end(),begin,reinterpret_cast<const u8*>(end));
            if(stage==g_GameManager.currentStage-1)
                std::memcpy(bytes.data()+header.stageReplayDataOffsets[stage],&g_GameManager.score,sizeof(g_GameManager.score));
        }
        header.score=g_GameManager.guiScore;
        const float ratio=g_Supervisor.unk1b8>0?g_Supervisor.unk1b4/g_Supervisor.unk1b8:1.f;
        header.slowdownRate=(1-std::clamp((ratio-.5f)*2,0.f,1.f))*100;
        header.slowdownRate2=header.slowdownRate+1.12f;
        header.slowdownRate3=header.slowdownRate+2.34f;
        const auto now=std::time(nullptr);
        const auto* date=std::localtime(&now);
        if(date)std::strftime(header.date,sizeof(header.date),"%m/%d/%y",date);
        std::memset(header.name,' ',sizeof(header.name));
        if(replayName)std::memcpy(header.name,replayName,std::min(std::strlen(replayName),sizeof(header.name)));
        // Saving must not advance the gameplay RNG or encrypt the live recording.
        auto rng=g_Rng;
        header.key=rng.GetRandomU16InRange(128)+64;
        header.rngValue3=rng.GetRandomU16InRange(256);
        header.rngValue1=rng.GetRandomU16InRange(256);
        header.rngValue2=rng.GetRandomU16InRange(256);
        std::memcpy(bytes.data(),&header,sizeof(header));
        u32 checksum=0x3f000318;
        for(size_t i=offsetof(ReplayHeader,key);i<bytes.size();i++)checksum+=bytes[i];
        std::memcpy(bytes.data()+offsetof(ReplayHeader,checksum),&checksum,sizeof(checksum));
        u8 offset=header.key;
        for(size_t i=offsetof(ReplayHeader,rngValue3);i<bytes.size();i++,offset+=7)bytes[i]+=offset;
        if(!ReplayStorage::Write(replayPath,bytes.data(),bytes.size()))return false;
    }
    if(!mgr->IsDemo())for(auto*& stage:mgr->replayData->stageReplayData) {
        if(stage)ReleaseStageReplayData(stage);
        stage=nullptr;
    }
    g_Chain.Cut(mgr->calcChain);
    return true;
}

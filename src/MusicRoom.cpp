#include "MusicRoom.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "SoundPlayer.hpp"
#include <algorithm>
#include "utils.hpp"
#ifdef __3DS__
#include "platform/3ds/CtrRenderer.hpp"
#endif
#include <cstdlib>
#include <cstring>

ZunResult MusicRoom::CheckInputEnable()
{
    if (this->waitFramesCount >= 8)
    {
        this->enableInput = 1;
    }

    return ZUN_SUCCESS;
}

bool MusicRoom::ProcessInput()
{
    i32 i;
    char lineCharBuffer[64];
    i32 listPos;

    if(this->numDescriptors<=0)return false;
    const int oldCursor=this->cursor;

    // This variable is never used after this?
    listPos = this->listingOffset;

    if (WAS_PRESSED(TH_BUTTON_UP))
    {
        this->cursor--;
        // Vertical wrap-around
        if (this->cursor < 0)
        {
            this->cursor = this->numDescriptors - 1;
            this->listingOffset = std::max<i32>(0,this->numDescriptors - 10);
        }
        // Scroll list up
        else if (this->listingOffset > this->cursor)
        {
            this->listingOffset = this->cursor;
        }
    }

    if (WAS_PRESSED(TH_BUTTON_DOWN))
    {
        this->cursor++;
        // Vertical wrap-around
        if (this->cursor >= this->numDescriptors)
        {
            this->cursor = 0;
            this->listingOffset = 0;
        }
        else
        {
            // Scroll list down
            if (this->listingOffset <= this->cursor - 10)
            {
                this->listingOffset = this->cursor - 9;
            }
        }
    }

    if(this->cursor!=oldCursor)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);

    if (WAS_PRESSED(TH_BUTTON_SELECTMENU))
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        this->selectedSongIndex = this->cursor;
        g_Supervisor.PlayAudio(this->trackDescriptors[this->selectedSongIndex].path);

        this->descriptionPage=0;
        this->DisplayDescription();
    }

    if(WAS_PRESSED(TH_BUTTON_LEFT) || WAS_PRESSED(TH_BUTTON_RIGHT)) {
        int lines=0;
        for(int row=0;row<32;row++)if(this->trackDescriptors[this->selectedSongIndex].description[row][0])lines=row+1;
        const int pages=std::max(1,(lines+7)/8);
        const int direction=WAS_PRESSED(TH_BUTTON_RIGHT)?1:-1;
        this->descriptionPage=(this->descriptionPage+direction+pages)%pages;
        this->DisplayDescription();g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    }

    if (WAS_PRESSED(TH_BUTTON_RETURNMENU))
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
        return true;
    }

    return false;
}

static MusicRoom g_MusicRoom;
ZunResult MusicRoom::RegisterChain()
{
    MusicRoom *musicRoom;

    musicRoom = &g_MusicRoom;
    std::memset(musicRoom, 0, sizeof(MusicRoom));

    musicRoom->calc_chain = g_Chain.CreateElem((ChainCallback)MusicRoom::OnUpdate);
    musicRoom->calc_chain->arg = musicRoom;
    musicRoom->calc_chain->addedCallback = (ChainAddedCallback)MusicRoom::AddedCallback;
    musicRoom->calc_chain->deletedCallback = (ChainDeletedCallback)MusicRoom::DeletedCallback;

    if (g_Chain.AddToCalcChain(musicRoom->calc_chain, TH_CHAIN_PRIO_CALC_MAINMENU))
    {
        return ZUN_ERROR;
    }

    musicRoom->draw_chain = g_Chain.CreateElem((ChainCallback)MusicRoom::OnDraw);
    musicRoom->draw_chain->arg = musicRoom;
    g_Chain.AddToDrawChain(musicRoom->draw_chain, TH_CHAIN_PRIO_DRAW_MAINMENU);

    return ZUN_SUCCESS;
}

ChainCallbackResult MusicRoom::OnUpdate(MusicRoom *musicRoom)
{
    i32 oldInputSetting = musicRoom->enableInput;
    for (;;)
    {
        switch (musicRoom->enableInput)
        {
        case false:
            if (!musicRoom->CheckInputEnable())
            {
                break;
            }

            continue;

        case true:
            if (musicRoom->ProcessInput())
            {
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
        }
        break;
    }

    if (oldInputSetting != musicRoom->enableInput)
    {
        musicRoom->waitFramesCount = 0;
    }
    else
    {
        musicRoom->waitFramesCount++;
    }
    g_AnmManager->ExecuteScript(musicRoom->mainVm);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult MusicRoom::OnDraw(MusicRoom *musicRoom)
{
    i32 i;
    ZunVec3 textPos;
    char rightArrowStr[4];

    rightArrowStr[0] = TEXT_RIGHT_ARROW;
    rightArrowStr[1] = '\0';

    g_AnmManager->SetCurrentTexture(0);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    g_AnmManager->DrawNoRotation(musicRoom->mainVm);

    // Draw the 10 songs in the song select window, and list indices
    for (i = musicRoom->listingOffset; i < std::min(musicRoom->numDescriptors,musicRoom->listingOffset + 10); i++)
    {
        if (musicRoom->cursor != i)
        {
            musicRoom->titleSprites[i].color = COLOR_SET_ALPHA(COLOR_GREY, 0xe0);
            g_AsciiManager.color = COLOR_SET_ALPHA(COLOR_GREY, 0xe0);
        }
        else
        {
            musicRoom->titleSprites[i].color = COLOR_WHITE;
            g_AsciiManager.color = COLOR_WHITE;
        }

        musicRoom->titleSprites[i].pos.x = 93.0f;
        musicRoom->titleSprites[i].pos.y = 104.0f + (((i + 1) - musicRoom->listingOffset) * 18) - 20.0f;
        musicRoom->titleSprites[i].pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(&musicRoom->titleSprites[i]);

        textPos = musicRoom->titleSprites[i].pos;
        textPos.x -= 60.0f;

        if (musicRoom->cursor == i)
        {
            g_AsciiManager.AddString(&textPos, rightArrowStr);
        }

        textPos.x += 15.0f;
        g_AsciiManager.AddFormatText(&textPos, "%2d.", i + 1);
    }

    i++; // ???

#ifdef __3DS__
    g_AnmManager->FlushVertexBuffer();
    auto* renderer=static_cast<CtrRenderer*>(g_GfxBackend);renderer->BeginMenuLayer();
#endif
    for (i = 0; i < ARRAY_SIZE_SIGNED(musicRoom->descriptionSprites); i++)
    {
#ifdef __3DS__
        auto& vm=musicRoom->descriptionSprites[i];
        const auto original=vm.pos;const float sx=vm.scaleX,sy=vm.scaleY;
        vm.pos.x=24+(i%2)*298;vm.pos.y=96+(i/2)*34;
        vm.scaleX*=1.2f;vm.scaleY*=1.2f;
        g_AnmManager->DrawNoRotation(&vm);vm.pos=original;vm.scaleX=sx;vm.scaleY=sy;
#else
        g_AnmManager->DrawNoRotation(&musicRoom->descriptionSprites[i]);
#endif
    }
#ifdef __3DS__
    g_AnmManager->FlushVertexBuffer();renderer->EndMenuLayer();
#endif

    g_AsciiManager.color = COLOR_WHITE;

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult MusicRoom::AddedCallback(MusicRoom *musicRoom)
{
    u32 charIndex;
    char *currChar;
    char *fileBase;
    i32 i;
    char lineCharBuffer[64];
    i32 lineIndex;

    if (g_AnmManager->LoadSurface(0, "data/result/music.jpg") != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (g_AnmManager->LoadAnm(ANM_FILE_MUSIC00, "data/music00.anm", ANM_OFFSET_MUSIC00) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (g_AnmManager->LoadAnm(ANM_FILE_MUSIC01, "data/music01.anm", ANM_OFFSET_MUSIC01) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (g_AnmManager->LoadAnm(ANM_FILE_MUSIC02, "data/music02.anm", ANM_OFFSET_MUSIC02) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    g_AnmManager->SetAndExecuteScriptIdx(musicRoom->mainVm, ANM_OFFSET_MUSIC00);
    musicRoom->waitFramesCount = 0;
    currChar = (char *)FileSystem::OpenPath("data/musiccmt.txt", 0);
    fileBase = currChar;

    if (currChar == NULL)
    {
        return ZUN_ERROR;
    }

    musicRoom->trackDescriptors = new TrackDescriptor[ARRAY_SIZE_SIGNED(musicRoom->titleSprites)]();

    i = -1;
    while (currChar - fileBase < (i32)g_LastFileSize)
    {
        if (*currChar == '@')
        {
            currChar++;
            if(currChar-fileBase>=static_cast<i32>(g_LastFileSize))goto finishMusiccmtRead;
            if(i+1>=ARRAY_SIZE_SIGNED(musicRoom->titleSprites))goto finishMusiccmtRead;
            i++;
            charIndex = 0;

            while (*currChar != '\n' && *currChar != '\r')
            {
                if(charIndex+1<sizeof(musicRoom->trackDescriptors[i].path))
                    musicRoom->trackDescriptors[i].path[charIndex] = *currChar;
                currChar++;
                charIndex++;
                if (currChar - fileBase >= (i32)g_LastFileSize)
                {
                    goto finishMusiccmtRead;
                }
            }

            while (*currChar == '\n' || *currChar == '\r')
            {
                currChar++;
                if (currChar - fileBase >= (i32)g_LastFileSize)
                {
                    goto finishMusiccmtRead;
                }
            }

            charIndex = 0;
            while (*currChar != '\n' && *currChar != '\r')
            {
                if(charIndex+1<sizeof(musicRoom->trackDescriptors[i].title))
                    musicRoom->trackDescriptors[i].title[charIndex] = *currChar;
                currChar++;
                charIndex++;
                if (currChar - fileBase >= (i32)g_LastFileSize)
                {
                    goto finishMusiccmtRead;
                }
            }

            // Begin descriptions at the first nonempty row after the title.
            while (*currChar == '\n' || *currChar == '\r')
            {
                currChar++;
                if (currChar - fileBase >= (i32)g_LastFileSize)
                {
                    goto finishMusiccmtRead;
                }
            }

            for (lineIndex = 0; lineIndex < 32; lineIndex++)
            {
                if (*currChar == '@')
                {
                    break;
                }

                std::memset(musicRoom->trackDescriptors[i].description[lineIndex], 0,
                            sizeof(musicRoom->trackDescriptors[i].description[lineIndex]));
                charIndex = 0;
                while (*currChar != '\n' && *currChar != '\r')
                {
                    if(charIndex+1<sizeof(musicRoom->trackDescriptors[i].description[lineIndex]))
                        musicRoom->trackDescriptors[i].description[lineIndex][charIndex] = *currChar;
                    currChar++;
                    charIndex++;
                    if (currChar - fileBase >= (i32)g_LastFileSize)
                    {
                        goto finishMusiccmtRead;
                    }
                }

                while (*currChar == '\n' || *currChar == '\r')
                {
                    currChar++;
                    if (currChar - fileBase >= (i32)g_LastFileSize)
                    {
                        goto finishMusiccmtRead;
                    }
                }
            }
        }
        else
        {
            currChar++;
        }
    }

finishMusiccmtRead:
    musicRoom->numDescriptors = i + 1;
    if(musicRoom->numDescriptors==0) {
        delete[] musicRoom->trackDescriptors;musicRoom->trackDescriptors=nullptr;
        std::free(fileBase);return ZUN_ERROR;
    }

    for (i = 0; i < musicRoom->numDescriptors; i++)
    {
        g_AnmManager->InitializeAndSetSprite(&musicRoom->titleSprites[i], ANM_OFFSET_MUSIC01 + i);
        g_AnmManager->DrawVmTextFmt(&musicRoom->titleSprites[i], COLOR_MUSIC_ROOM_SONG_TITLE_TEXT,
                                    COLOR_MUSIC_ROOM_SONG_TITLE_SHADOW, "%s", musicRoom->trackDescriptors[i].title);
        musicRoom->titleSprites[i].pos.x = 93.0f;
        musicRoom->titleSprites[i].pos.y = 104.0f + ((i + 1) * 18) - 20.0f;
        musicRoom->titleSprites[i].pos.z = 0.0f;
        musicRoom->titleSprites[i].flags.anchor = AnmVmAnchor_TopLeft;
    }

    for(i=0;i<ARRAY_SIZE_SIGNED(musicRoom->descriptionSprites);i++)
        g_AnmManager->InitializeAndSetSprite(&musicRoom->descriptionSprites[i],ANM_SCRIPT_TEXT_MUSIC_ROOM_DESC+i);
    musicRoom->descriptionPage=0;musicRoom->DisplayDescription();

    std::free(fileBase);

    return ZUN_SUCCESS;
}

void MusicRoom::DisplayDescription() {
    for(int i=0;i<ARRAY_SIZE_SIGNED(this->descriptionSprites);i++) {
        char line[33]{};
        const auto* source=this->trackDescriptors[this->selectedSongIndex].description[this->descriptionPage*8+i/2];
        if(i%2==0 || std::strlen(source)>32)std::memcpy(line,source+(i%2)*32,32);
        auto& sprite=this->descriptionSprites[i];sprite.flags.flag1=line[0]!=0;
        if(line[0])g_AnmManager->DrawVmTextFmt(&sprite,COLOR_MUSIC_ROOM_SONG_DESC_TEXT,COLOR_MUSIC_ROOM_SONG_DESC_SHADOW,"%s",line);
        sprite.pos=ZunVec3((i%2)*248.f+96.f,320.f+(i/2)*16.f,0.f);
        sprite.flags.anchor=AnmVmAnchor_TopLeft;
    }
}

ZunResult MusicRoom::DeletedCallback(MusicRoom *musicRoom)
{
    delete[] musicRoom->trackDescriptors;
    musicRoom->trackDescriptors = NULL;

    g_AnmManager->ReleaseSurface(0);
    g_AnmManager->ReleaseAnm(ANM_FILE_MUSIC00);
    g_AnmManager->ReleaseAnm(ANM_FILE_MUSIC01);
    g_AnmManager->ReleaseAnm(ANM_FILE_MUSIC02);
    g_Chain.Cut(musicRoom->draw_chain);
    musicRoom->draw_chain = NULL;

    return ZUN_SUCCESS;
}

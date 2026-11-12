#include "MusicRoom.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "SoundPlayer.hpp"
#include "TextHelper.hpp"
#include <algorithm>
#include "utils.hpp"
#ifdef __3DS__
#include "platform/3ds/CtrRenderer.hpp"
#include "platform/3ds/MusicControls.hpp"
#include "platform/3ds/MusicDescription.hpp"
#include "platform/3ds/NdspAudio.hpp"
#include "platform/3ds/Settings.hpp"
#include "platform/3ds/Touch.hpp"
#endif
#include <cstdlib>
#include <cstring>

namespace {
#ifdef __3DS__
// The title ends near y = 80. Rows leave an equal margin below it and above the bottom edge.
constexpr int VisibleRows=14;
constexpr float ListScale=1.4f;
constexpr float ListTop=106,ListSpacing=25;
constexpr float ListArrowX=8,ListNumberX=32,ListTitleX=98;
constexpr float DescriptionScale=1.2f;
constexpr float DescriptionLeft=24,DescriptionTop=36,DescriptionSpacing=34;
constexpr float DescriptionRowWidth=(640-2*DescriptionLeft)/DescriptionScale;
constexpr int DescriptionRowsPerPage=8;
constexpr int MaxDescriptionRows=64;
#else
constexpr int VisibleRows=10;
#endif
}

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

    listPos = this->listingOffset;

    if (WAS_PRESSED(TH_BUTTON_UP))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor = this->numDescriptors - 1;
            this->listingOffset = std::max<i32>(0,this->numDescriptors - VisibleRows);
        }
        else if (this->listingOffset > this->cursor)
        {
            this->listingOffset = this->cursor;
        }
    }

    if (WAS_PRESSED(TH_BUTTON_DOWN))
    {
        this->cursor++;
        if (this->cursor >= this->numDescriptors)
        {
            this->cursor = 0;
            this->listingOffset = 0;
        }
        else
        {
            if (this->listingOffset <= this->cursor - VisibleRows)
            {
                this->listingOffset = this->cursor - VisibleRows + 1;
            }
        }
    }

    if(this->cursor!=oldCursor)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);

    if (WAS_PRESSED(TH_BUTTON_SELECTMENU))
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        this->PlayTrack(this->cursor);
    }

    if(WAS_PRESSED(TH_BUTTON_LEFT) || WAS_PRESSED(TH_BUTTON_RIGHT)) {
        const int direction=WAS_PRESSED(TH_BUTTON_RIGHT)?1:-1;
        this->descriptionPage=(this->descriptionPage+direction+this->descriptionPages)%this->descriptionPages;
        this->DisplayDescription();g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    }

#ifdef __3DS__
    int touchX=0,touchY=0;
    if(CtrTouch::Tap(touchX,touchY))this->HandleTouch(touchX,touchY);
#endif

    if (WAS_PRESSED(TH_BUTTON_RETURNMENU))
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
        return true;
    }

    return false;
}

void MusicRoom::PlayTrack(i32 index)
{
    this->selectedSongIndex = index;
    this->cursor = index;
    if (this->cursor < this->listingOffset)
    {
        this->listingOffset = this->cursor;
    }
    else if (this->cursor >= this->listingOffset + VisibleRows)
    {
        this->listingOffset = this->cursor - VisibleRows + 1;
    }
    g_Supervisor.PlayAudio(this->trackDescriptors[index].path);
    this->descriptionPage = 0;
    this->DisplayDescription();
}

#ifdef __3DS__
void MusicRoom::HandleTouch(int x, int y)
{
    using CtrMusicControls::Action;
    float fraction = 0;
    switch (CtrMusicControls::Hit(x, y, fraction))
    {
    case Action::None:
        return;
    case Action::Previous:
        this->PlayTrack((this->selectedSongIndex + this->numDescriptors - 1) % this->numDescriptors);
        break;
    case Action::Next:
        this->PlayTrack((this->selectedSongIndex + 1) % this->numDescriptors);
        break;
    case Action::StartStop:
        if (float seconds, duration; CtrAudio::MusicPosition(seconds, duration))
            g_Supervisor.StopAudio();
        else
            this->PlayTrack(this->selectedSongIndex);
        break;
    case Action::Seek:
        CtrAudio::SeekMusic(fraction);
        return;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
}
#endif

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
#ifdef __3DS__
    // The English title is stored at half width to fit its 256 px sprite.
    const float titleScaleX = musicRoom->mainVm->scaleX;
    if (CtrSettings::English())
    {
        musicRoom->mainVm->scaleX *= 2;
    }
    g_AnmManager->DrawNoRotation(musicRoom->mainVm);
    musicRoom->mainVm->scaleX = titleScaleX;
#else
    g_AnmManager->DrawNoRotation(musicRoom->mainVm);
#endif

    for (i = musicRoom->listingOffset; i < std::min(musicRoom->numDescriptors,musicRoom->listingOffset + VisibleRows); i++)
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

#ifdef __3DS__
        musicRoom->titleSprites[i].pos.x = ListTitleX;
        musicRoom->titleSprites[i].pos.y = ListTop + (i - musicRoom->listingOffset) * ListSpacing;
        musicRoom->titleSprites[i].pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(&musicRoom->titleSprites[i]);

        g_AsciiManager.scale = ZunVec2(ListScale, ListScale);
        textPos = musicRoom->titleSprites[i].pos;
        if (musicRoom->cursor == i)
        {
            textPos.x = ListArrowX;
            g_AsciiManager.AddString(&textPos, rightArrowStr);
        }
        textPos.x = ListNumberX;
        g_AsciiManager.AddFormatText(&textPos, "%2d.", i + 1);
        g_AsciiManager.scale = ZunVec2(1.0f, 1.0f);
#else
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
#endif
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
        const float sx=vm.scaleX,sy=vm.scaleY;
        vm.scaleX*=DescriptionScale;vm.scaleY*=DescriptionScale;
        g_AnmManager->DrawNoRotation(&vm);vm.scaleX=sx;vm.scaleY=sy;
#else
        g_AnmManager->DrawNoRotation(&musicRoom->descriptionSprites[i]);
#endif
    }
#ifdef __3DS__
    CtrMusicControls::DrawGauge();
    g_AnmManager->FlushVertexBuffer();renderer->EndMenuLayer();
    CtrMusicControls::Draw();
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

#ifdef __3DS__
    constexpr const char *background = "data/result/result.jpg";
#else
    constexpr const char *background = "data/result/music.jpg";
#endif
    if (g_AnmManager->LoadSurface(0, background) != ZUN_SUCCESS)
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

            while (*currChar == '\n' || *currChar == '\r')
            {
                currChar++;
                if (currChar - fileBase >= (i32)g_LastFileSize)
                {
                    goto finishMusiccmtRead;
                }
            }

#ifdef __3DS__
            // The Japanese comments open with "No.N" and the title, which the list already shows.
            if (currChar - fileBase + 3 <= (i32)g_LastFileSize && std::strncmp(currChar, "No.", 3) == 0)
            {
                while (*currChar != '\n' && *currChar != '\r')
                {
                    currChar++;
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
#endif

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

#ifdef __3DS__
    // English titles and comments use the Japanese MS Gothic text style.
    TextHelper::SetOriginalFont(true);
#endif
    for (i = 0; i < musicRoom->numDescriptors; i++)
    {
        g_AnmManager->InitializeAndSetSprite(&musicRoom->titleSprites[i], ANM_OFFSET_MUSIC01 + i);
        g_AnmManager->DrawVmTextFmt(&musicRoom->titleSprites[i], COLOR_MUSIC_ROOM_SONG_TITLE_TEXT,
                                    COLOR_MUSIC_ROOM_SONG_TITLE_SHADOW, "%s", musicRoom->trackDescriptors[i].title);
        musicRoom->titleSprites[i].pos.x = 93.0f;
        musicRoom->titleSprites[i].pos.y = 104.0f + ((i + 1) * 18) - 20.0f;
        musicRoom->titleSprites[i].pos.z = 0.0f;
        musicRoom->titleSprites[i].flags.anchor = AnmVmAnchor_TopLeft;
#ifdef __3DS__
        musicRoom->titleSprites[i].scaleX = ListScale;
        musicRoom->titleSprites[i].scaleY = ListScale;
#endif
    }

    for(i=0;i<ARRAY_SIZE_SIGNED(musicRoom->descriptionSprites);i++)
        g_AnmManager->InitializeAndSetSprite(&musicRoom->descriptionSprites[i],ANM_SCRIPT_TEXT_MUSIC_ROOM_DESC+i);
    musicRoom->descriptionPage=0;musicRoom->DisplayDescription();
#ifdef __3DS__
    TextHelper::SetOriginalFont(false);
#endif

    std::free(fileBase);

    return ZUN_SUCCESS;
}

void MusicRoom::DisplayDescription() {
    const auto& track=this->trackDescriptors[this->selectedSongIndex];
    int lines=0;
    for(int row=0;row<32;row++)if(track.description[row][0])lines=row+1;
#ifdef __3DS__
    TextHelper::SetOriginalFont(true);
    const auto& first=this->descriptionSprites[0];
    const int fontHeight=first.fontWidth<=0?15:first.fontWidth;
    CtrMusicDescription::Row rows[MaxDescriptionRows];
    const int rowCount=CtrMusicDescription::Layout(track.description,lines,fontHeight,first.sprite->widthPx,
        DescriptionRowWidth,rows,MaxDescriptionRows);
    this->descriptionPages=std::max(1,(rowCount+DescriptionRowsPerPage-1)/DescriptionRowsPerPage);
    for(int i=0;i<ARRAY_SIZE_SIGNED(this->descriptionSprites);i++) {
        auto& sprite=this->descriptionSprites[i];
        const int row=this->descriptionPage*DescriptionRowsPerPage+i/2;
        sprite.flags.flag1=row<rowCount && i%2<rows[row].partCount;
        if(!sprite.flags.flag1)continue;
        const auto& part=rows[row].parts[i%2];
        g_AnmManager->DrawVmTextFmt(&sprite,COLOR_MUSIC_ROOM_SONG_DESC_TEXT,COLOR_MUSIC_ROOM_SONG_DESC_SHADOW,"%s",part.text);
        sprite.pos=ZunVec3(DescriptionLeft+part.x*DescriptionScale,DescriptionTop+(i/2)*DescriptionSpacing,0.f);
        sprite.flags.anchor=AnmVmAnchor_TopLeft;
    }
    TextHelper::SetOriginalFont(false);
#else
    this->descriptionPages=std::max(1,(lines+7)/8);
    for(int i=0;i<ARRAY_SIZE_SIGNED(this->descriptionSprites);i++) {
        char line[33]{};
        const auto* source=track.description[this->descriptionPage*8+i/2];
        if(i%2==0 || std::strlen(source)>32)std::memcpy(line,source+(i%2)*32,32);
        auto& sprite=this->descriptionSprites[i];sprite.flags.flag1=line[0]!=0;
        if(line[0])g_AnmManager->DrawVmTextFmt(&sprite,COLOR_MUSIC_ROOM_SONG_DESC_TEXT,COLOR_MUSIC_ROOM_SONG_DESC_SHADOW,"%s",line);
        sprite.pos=ZunVec3((i%2)*248.f+96.f,320.f+(i/2)*16.f,0.f);
        sprite.flags.anchor=AnmVmAnchor_TopLeft;
    }
#endif
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

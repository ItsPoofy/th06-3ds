#include "SoundPlayer.hpp"
#ifdef __3DS__
#include "platform/3ds/VorbisStream.hpp"
#include "platform/3ds/Runtime.hpp"
#include "platform/3ds/Settings.hpp"
#include "platform/3ds/AudioTiming.hpp"
#endif

#include "FileSystem.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"
#include "utils.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <vector>

// This would all be a lot easier with SDL_mixer, but SDL_mixer doesn't permit any way of doing custom
//   loop points that would be accurate to the sample like EoSD needs. So instead we get to read WAVs and
//   mix everything by hand. Yay

#define BACKGROUND_MUSIC_WAV_NUM_CHANNELS 2
#define BACKGROUND_MUSIC_WAV_SAMPLE_RATE 44100
#define BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE 16
#define BACKGROUND_MUSIC_WAV_BLOCK_ALIGN (BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE / 8 * BACKGROUND_MUSIC_WAV_NUM_CHANNELS)
#define BACKGROUND_MUSIC_WAV_BYTE_RATE (BACKGROUND_MUSIC_WAV_BLOCK_ALIGN * BACKGROUND_MUSIC_WAV_SAMPLE_RATE)

// DirectSound deals with volume by subtracting a number measured in hundredths of decibels from the source sound.
//   The scale is from 0 (no volume modification) to -10,000 (subtraction of 100 decibels, and basically silent).
//   20 decibels affects wave amplitude by a factor of 10

static const SoundBufferIdxVolume g_SoundBufferIdxVol[32] = {
    {0, -1500}, {0, -2000}, {1, -1200}, {1, -1400}, {2, -1000},  {3, -500},   {4, -500},   {5, -1700},
    {6, -1700}, {7, -1700}, {8, -1000}, {9, -1000}, {10, -1900}, {11, -1200}, {12, -900},  {5, -1500},
    {13, -900}, {14, -900}, {15, -600}, {16, -400}, {17, -1100}, {18, -900},  {5, -1800},  {6, -1800},
    {7, -1800}, {19, -300}, {20, -600}, {21, -800}, {22, -100},  {23, -500},  {24, -1000}, {25, -1000},
};
static const char *const g_SFXList[26] = {
    "data/wav/plst00.wav", "data/wav/enep00.wav",   "data/wav/pldead00.wav", "data/wav/power0.wav",
    "data/wav/power1.wav", "data/wav/tan00.wav",    "data/wav/tan01.wav",    "data/wav/tan02.wav",
    "data/wav/ok00.wav",   "data/wav/cancel00.wav", "data/wav/select00.wav", "data/wav/gun00.wav",
    "data/wav/cat00.wav",  "data/wav/lazer00.wav",  "data/wav/lazer01.wav",  "data/wav/enep01.wav",
    "data/wav/nep00.wav",  "data/wav/damage00.wav", "data/wav/item00.wav",   "data/wav/kira00.wav",
    "data/wav/kira01.wav", "data/wav/kira02.wav",   "data/wav/extend.wav",   "data/wav/timeout.wav",
    "data/wav/graze.wav",  "data/wav/powerup.wav",
};
SoundPlayer g_SoundPlayer;

SoundPlayer::SoundPlayer()
{
    // Note: memset of an std::mutex crashes on windows
    // std::memset(this, 0, sizeof(SoundPlayer));
}

ZunResult SoundPlayer::InitializeDSound()
{
    SDL_AudioSpec desiredAudio{};
    SDL_AudioSpec obtainedAudio{};

    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        goto fail;
    }

    desiredAudio.freq = 44100;
    desiredAudio.format = AUDIO_S16SYS;
    desiredAudio.channels = 2;
#ifdef __3DS__
    desiredAudio.samples = CtrAudio::DeviceFrames;
#else
    desiredAudio.samples = 2048;
#endif
    desiredAudio.padding = 0;
    desiredAudio.callback = NULL;

    this->audioDev = SDL_OpenAudioDevice(NULL, 0, &desiredAudio, &obtainedAudio, 0);

    if (this->audioDev == 0)
    {
        goto fail;
    }

    this->terminateFlag=false;
#ifdef __3DS__
    this->backgroundMusicThreadHandle=SDL_CreateThread([](void* p) -> int {
        ((SoundPlayer*)p)->BackgroundMusicPlayerThread();return 0;
    }, "th06-mixer",this);
    if(!this->backgroundMusicThreadHandle) { SDL_CloseAudioDevice(audioDev);audioDev=0;goto fail; }
#else
    this->backgroundMusicThreadHandle = std::thread(&SoundPlayer::BackgroundMusicPlayerThread, this);
#endif

    g_GameErrorContext.Log(TH_DBG_SOUNDPLAYER_INIT_SUCCESS);
#ifdef __3DS__
    CtrRuntime::Log("Audio: stereo output and mixer initialized");
#endif
    return ZUN_SUCCESS;

fail:
#ifdef __3DS__
    CtrRuntime::Log(SDL_GetError());
#endif
    g_GameErrorContext.Log(TH_ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT);
    return ZUN_ERROR;
}

ZunResult SoundPlayer::Release(void)
{
    this->terminateFlag = true;
#ifdef __3DS__
    if(backgroundMusicThreadHandle) SDL_WaitThread(backgroundMusicThreadHandle,nullptr);
    backgroundMusicThreadHandle=nullptr;
#else
    if(this->backgroundMusicThreadHandle.joinable()) this->backgroundMusicThreadHandle.join();
#endif
    this->terminateFlag = false;

    StopBGM();

    for (int i = 0; i < ARRAY_SIZE_SIGNED(this->soundBuffers); i++)
    {
        if (this->soundBuffers[i].samples != NULL)
        {
            delete[] this->soundBuffers[i].samples;
            this->soundBuffers[i].samples = NULL;
            this->soundBuffers[i].isPlaying = false;
        }
    }

    if (this->audioDev != 0)
    {
        SDL_CloseAudioDevice(this->audioDev);
        this->audioDev = 0;
    }

    return ZUN_SUCCESS;
}

void SoundPlayer::StopBGM()
{
    this->BgmMutex().lock();
    bgmPlaying=false;
    if (this->backgroundMusic.srcWav.fileStream != NULL)
    {
        SDL_RWclose(this->backgroundMusic.srcWav.fileStream);
        this->backgroundMusic.srcWav.fileStream = NULL;

        utils::DebugPrint2("stop BGM\n");
    }
    this->BgmMutex().unlock();
}

void SoundPlayer::FadeOut(f32 seconds)
{
    BgmMutex().lock();
    if (this->backgroundMusic.srcWav.fileStream != NULL)
    {
        this->backgroundMusic.fadeoutLen = seconds * 44100;
        this->backgroundMusic.fadeoutProgress = 0;
    }
    BgmMutex().unlock();
}

ZunResult SoundPlayer::LoadWav(const char *path)
{
    SDL_RWops *fileStream;
    char idBuf[4];
    u32 riffSize;
    u32 wavDataSize;

    if (this->audioDev == 0)
    {
        return ZUN_ERROR;
    }

    if (g_Supervisor.cfg.playSounds == 0)
    {
        return ZUN_ERROR;
    }

    this->StopBGM();

    utils::DebugPrint2("load BGM\n");

#ifdef __3DS__
    fileStream=OpenVorbisPCM(path);
    if(!fileStream) {CtrRuntime::Log("Audio: music decode/open failed");CtrRuntime::Log(path);return ZUN_ERROR;}
    BgmMutex().lock();
    backgroundMusic={};
    bgmPlaying=false;
    backgroundMusic.srcWav.fileStream=fileStream;
    backgroundMusic.srcWav.samples=SDL_RWsize(fileStream)/4;
    backgroundMusic.loopEnd=backgroundMusic.srcWav.samples;
    BgmMutex().unlock();
    CtrRuntime::Log("Audio: music stream loaded");
    CtrRuntime::Log(path);
    return ZUN_SUCCESS;
#else
    fileStream = SDL_RWFromFile(path, "rb");
#endif

    if (fileStream == NULL)
    {
        utils::DebugPrint2("error : wav file load error %s\n", path);
        return ZUN_ERROR;
    }

    // Minimum size of RIFF header and chunk info preceeding the sample data
    if (SDL_RWsize(fileStream) < 44)
    {
        goto fail;
    }

    if (SDL_RWread(fileStream, idBuf, 4, 1) != 1 || std::strncmp(idBuf, "RIFF", 4) != 0)
    {
        goto fail;
    }

    riffSize = SDL_ReadLE32(fileStream);

    // Same bounds check done earlier on the total filesize
    if (riffSize < 36 || riffSize > SDL_RWsize(fileStream) - 8)
    {
        goto fail;
    }

    if (SDL_RWread(fileStream, idBuf, 4, 1) != 1 || std::strncmp(idBuf, "WAVE", 4) != 0)
    {
        goto fail;
    }

    // Checks here are quite a bit less flexible than what WAV can represent. EoSD uses 44.1 kHz, stereo, 16-bit PCM
    //   so that's what we handle. We also assume that fmt and data are the only subchunks, which is definitely not
    //   a general guarantee, but it'll work fine with EoSD's WAV files.

    if (SDL_RWread(fileStream, idBuf, 4, 1) != 1 || std::strncmp(idBuf, "fmt ", 4) != 0)
    {
        goto fail;
    }

    // Format subchunk size. Guaranteed 16 for PCM data
    if (SDL_ReadLE32(fileStream) != 16)
    {
        goto fail;
    }

    // Audio format. 1 represents raw PCM samples
    if (SDL_ReadLE16(fileStream) != 1)
    {
        goto fail;
    }

    // Number of channels. We expect stereo
    if (SDL_ReadLE16(fileStream) != BACKGROUND_MUSIC_WAV_NUM_CHANNELS)
    {
        goto fail;
    }

    // Sample frequency rate
    if (SDL_ReadLE32(fileStream) != BACKGROUND_MUSIC_WAV_SAMPLE_RATE)
    {
        goto fail;
    }

    // Byte rate
    if (SDL_ReadLE32(fileStream) != BACKGROUND_MUSIC_WAV_BYTE_RATE)
    {
        goto fail;
    }

    // Block alignment
    if (SDL_ReadLE16(fileStream) != BACKGROUND_MUSIC_WAV_BLOCK_ALIGN)
    {
        goto fail;
    }

    // Bits per sample
    if (SDL_ReadLE16(fileStream) != BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE)
    {
        goto fail;
    }

    if (SDL_RWread(fileStream, idBuf, 4, 1) != 1 || std::strncmp(idBuf, "data", 4) != 0)
    {
        goto fail;
    }

    wavDataSize = SDL_ReadLE32(fileStream);

    if (wavDataSize > riffSize - 44)
    {
        goto fail;
    }

    this->backgroundMusic.srcWav.samples = wavDataSize / BACKGROUND_MUSIC_WAV_BLOCK_ALIGN;

    if (this->backgroundMusic.srcWav.samples == 0)
    {
        goto fail;
    }

    this->backgroundMusic.srcWav.fileStream = fileStream;
    this->backgroundMusic.srcWav.dataStartOffset = SDL_RWtell(fileStream);
    this->backgroundMusic.loopStart = 0;
    this->backgroundMusic.loopEnd = this->backgroundMusic.srcWav.samples;
    this->backgroundMusic.fadeoutLen = 0;
    this->backgroundMusic.fadeoutProgress = 0;
    this->backgroundMusic.pos = 0;

    return ZUN_SUCCESS;

fail:
    SDL_RWclose(fileStream);
    return ZUN_ERROR;
}

ZunResult SoundPlayer::LoadPos(const char *path)
{
    u8 *fileData;

    if (this->audioDev == 0
#ifndef __3DS__
        || g_Supervisor.cfg.playSounds == 0
#endif
       )
    {
        return ZUN_ERROR;
    }

    fileData = FileSystem::OpenPath(path, 0);

    if (fileData == NULL)
    {
        return ZUN_ERROR;
    }

    u32 loopStart,loopEnd;
    std::memcpy(&loopStart,fileData,4);std::memcpy(&loopEnd,fileData+4,4);
    BgmMutex().lock();
    if(!backgroundMusic.srcWav.fileStream) {free(fileData);BgmMutex().unlock();return ZUN_ERROR;}
    this->backgroundMusic.loopStart = SDL_SwapLE32(loopStart);
    this->backgroundMusic.loopEnd = SDL_SwapLE32(loopEnd);

    free(fileData);

    if (this->backgroundMusic.loopStart >= this->backgroundMusic.loopEnd ||
        this->backgroundMusic.loopEnd > this->backgroundMusic.srcWav.samples)
    {
        this->backgroundMusic.loopStart = 0;
        this->backgroundMusic.loopEnd = this->backgroundMusic.srcWav.samples;
        BgmMutex().unlock();
        return ZUN_ERROR;
    }

    BgmMutex().unlock();
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::InitSoundBuffers()
{
    if (this->audioDev == 0)
    {
        return ZUN_ERROR;
    }

    std::fill_n(this->soundBuffersToPlay, ARRAY_SIZE(this->soundBuffersToPlay), -1);

    for (int idx = 0; idx < ARRAY_SIZE_SIGNED(g_SoundBufferIdxVol); idx++)
    {
        if (this->LoadSound(idx, g_SFXList[g_SoundBufferIdxVol[idx].bufferIdx],
                            1.0f / ZUN_POWF(10.0f, (float)g_SoundBufferIdxVol[idx].volume / -2000)) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log(TH_ERR_SOUNDPLAYER_FAILED_TO_LOAD_SOUND_FILE, g_SFXList[idx]);
            return ZUN_ERROR;
        }

        this->soundBuffers[idx].isPlaying = false;
        this->soundBuffers[idx].pos = 0;
    }

    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::LoadSound(i32 idx, const char *path, f32 volumeMultiplier)
{
    SDL_AudioCVT sampleConversionDesc;
    SDL_AudioSpec wavFormat;
    u8 *wavRawData;
    u8 *wavRawSamples;
    u32 wavRawSampleByteCount;

    soundBufMutex.lock();

    if (this->soundBuffers[idx].samples != NULL)
    {
        delete[] this->soundBuffers[idx].samples;
        this->soundBuffers[idx].samples = NULL;
    }

    wavRawData = (u8 *)FileSystem::OpenPath(path, 0);

    if (wavRawData == NULL)
    {
        goto fail;
    }

    if (SDL_LoadWAV_RW(SDL_RWFromConstMem(wavRawData, g_LastFileSize), 1, &wavFormat, &wavRawSamples,
                       &wavRawSampleByteCount) == NULL)
    {
        g_GameErrorContext.Log(TH_ERR_NOT_A_WAV_FILE, path);
        goto fail;
    }

    // EoSD's sound files are all 22050 Hz, and some even use 8-bit samples. Converting them
    //   here only uses a few hundred extra kilobytes of RAM compared to the original code,
    //   but it might be worth looking into avoiding it for especially RAM-limited systems

    if (SDL_BuildAudioCVT(&sampleConversionDesc, wavFormat.format, wavFormat.channels, wavFormat.freq, AUDIO_S16SYS, 1,
                          44100) == 1)
    {
        sampleConversionDesc.len = wavRawSampleByteCount;
        sampleConversionDesc.buf = new u8[wavRawSampleByteCount * sampleConversionDesc.len_mult];
        std::memcpy(sampleConversionDesc.buf, wavRawSamples, wavRawSampleByteCount);

        SDL_ConvertAudio(&sampleConversionDesc);

        this->soundBuffers[idx].len = sampleConversionDesc.len_cvt / 2;
        this->soundBuffers[idx].samples = new i16[this->soundBuffers[idx].len];
        std::memcpy(this->soundBuffers[idx].samples, sampleConversionDesc.buf, sampleConversionDesc.len_cvt);

        delete[] sampleConversionDesc.buf;
    }
    else
    {
        this->soundBuffers[idx].len = wavRawSampleByteCount / 2;
        this->soundBuffers[idx].samples = new i16[this->soundBuffers[idx].len];
        std::memcpy(this->soundBuffers[idx].samples, wavRawSamples, wavRawSampleByteCount);
    }

    SDL_FreeWAV(wavRawSamples);

    for (u32 i = 0; i < this->soundBuffers[idx].len; i++)
    {
        this->soundBuffers[idx].samples[i] *= volumeMultiplier;
    }

    this->soundBuffers[idx].pos = 0;
    this->soundBuffers[idx].isPlaying = false;

    soundBufMutex.unlock();
    return ZUN_SUCCESS;

fail:
    soundBufMutex.unlock();
    return ZUN_ERROR;
}

ZunResult SoundPlayer::PlayBGM(bool isLooping)
{
    utils::DebugPrint2("play BGM\n");

    BgmMutex().lock();
    if (this->backgroundMusic.srcWav.fileStream == NULL)
    {
        BgmMutex().unlock();
        return ZUN_ERROR;
    }

    //    res = this->backgroundMusic->Reset();
    //    if (FAILED(res))
    //    {
    //        return ZUN_ERROR;
    //    }
    //
    //    buffer = this->backgroundMusic->GetBuffer(0);
    //    res = this->backgroundMusic->FillBufferWithSound(buffer, isLooping);
    //    if (FAILED(res))
    //    {
    //        return ZUN_ERROR;
    //    }
    //    res = this->backgroundMusic->Play(0, DSBPLAY_LOOPING);
    //    if (FAILED(res))
    //    {
    //        return ZUN_ERROR;
    //    }
    utils::DebugPrint2("comp\n");
    this->isLooping = isLooping;
    bgmPlaying=true;
    BgmMutex().unlock();
    return ZUN_SUCCESS;
}

void SoundPlayer::PlaySounds()
{
    i32 idx;
    i32 sndBufIdx;

    if (this->audioDev == 0
#ifndef __3DS__
        || !g_Supervisor.cfg.playSounds
#endif
       )
    {
        return;
    }

    soundBufMutex.lock();

    for (idx = 0; idx < ARRAY_SIZE_SIGNED(this->soundBuffersToPlay); idx++)
    {
        if (this->soundBuffersToPlay[idx] < 0)
        {
            break;
        }

        sndBufIdx = this->soundBuffersToPlay[idx];
        this->soundBuffersToPlay[idx] = -1;

        if (this->soundBuffers[sndBufIdx].samples == NULL)
        {
            continue;
        }

        this->soundBuffers[sndBufIdx].pos = 0;
        this->soundBuffers[sndBufIdx].isPlaying = true;
    }

    soundBufMutex.unlock();
}

void SoundPlayer::PlaySoundByIdx(SoundIdx idx)
{
#ifdef __3DS__
    // UI feedback must not compete with the original three-effect frame queue.
    // Touch selection also invokes this path, with the same vanilla samples.
    if(idx==SOUND_SELECT || idx==SOUND_BACK || idx==SOUND_MOVE_MENU) {
        soundBufMutex.lock();
        auto& buffer=soundBuffers[idx];
        if(buffer.samples && buffer.len) {buffer.pos=0;buffer.isPlaying=true;}
        soundBufMutex.unlock();return;
    }
#endif
    u32 i;

    for (i = 0; i < ARRAY_SIZE(this->soundBuffersToPlay); i++)
    {
        if (this->soundBuffersToPlay[i] < 0)
        {
            break;
        }

        if (this->soundBuffersToPlay[i] == idx)
        {
            return;
        }
    }

    if (i >= 3)
    {
        return;
    }

    this->soundBuffersToPlay[i] = idx;
}

void SoundPlayer::MixAudio(u32 samples)
{
    // This function has one owner (the mixer thread). Reuse its storage.
    static std::vector<i16> finalBuffer;
    static std::vector<i32> mixBuffer;
    static std::vector<i16> pcm;
    finalBuffer.resize(samples);mixBuffer.assign(samples,0);pcm.resize(samples);
    u8 playingChannels = 0;

    this->soundBufMutex.lock();

    for (int i = 0; i < ARRAY_SIZE_SIGNED(this->soundBuffers); i++)
    {
        if (!this->soundBuffers[i].isPlaying)
        {
            continue;
        }

        playingChannels++;

        // Sounds are all mono, so we need to duplicate each sample for stereo output
        const u32 samplesToMix = std::min(samples / 2, this->soundBuffers[i].len - this->soundBuffers[i].pos);

        for (u32 j = 0; j < samplesToMix; j++)
        {
            mixBuffer[j * 2] += this->soundBuffers[i].samples[this->soundBuffers[i].pos + j];
            mixBuffer[j * 2 + 1] += this->soundBuffers[i].samples[this->soundBuffers[i].pos + j];
        }

        this->soundBuffers[i].pos += samplesToMix;

        if (this->soundBuffers[i].pos == this->soundBuffers[i].len)
        {
            this->soundBuffers[i].isPlaying = false;
        }
    }

#ifdef __3DS__
    // Music decode must not block the game's sound-effect queue.
    soundBufMutex.unlock();
    const int sfxVolume=CtrSettings::SfxVolume();
    if(sfxVolume!=100)for(auto& sample:mixBuffer)sample=sample*sfxVolume/100;
    BgmMutex().lock();
#endif
    if (bgmPlaying && this->backgroundMusic.srcWav.fileStream != NULL)
    {
        u32 samplesMixed = 0;
        f32 fadeoutMult;

        if (this->backgroundMusic.fadeoutLen != 0)
        {
            f32 fadeoutInterp =
                mapRange(this->backgroundMusic.fadeoutProgress, 0, this->backgroundMusic.fadeoutLen, 0, 5);
            fadeoutMult = 1.0f / ZUN_POWF(10.0f, fadeoutInterp / 2.0f);
        }
        else
        {
            fadeoutMult = 1.0f;
        }


#ifdef __3DS__
        fadeoutMult*=CtrSettings::BgmVolume()*.01f;
#endif
        while (samplesMixed < samples / 2)
        {
            const u32 samplesToMix =
                std::min((samples / 2) - samplesMixed, this->backgroundMusic.loopEnd - this->backgroundMusic.pos);

            const u32 framesRead=SDL_RWread(backgroundMusic.srcWav.fileStream,pcm.data(),4,samplesToMix);
            if(framesRead!=samplesToMix) {
                SDL_RWclose(backgroundMusic.srcWav.fileStream);
                backgroundMusic.srcWav.fileStream=nullptr;
                break;
            }
            for (u32 j = 0; j < samplesToMix*2; j++)
                mixBuffer[samplesMixed*2+j] += ((i16)SDL_SwapLE16(pcm[j]))*fadeoutMult;

            this->backgroundMusic.pos += samplesToMix;
            samplesMixed += samplesToMix;

            if (this->backgroundMusic.pos == this->backgroundMusic.loopEnd)
            {
                if (this->isLooping)
                {
                    this->backgroundMusic.pos = this->backgroundMusic.loopStart;
                    if(SDL_RWseek(this->backgroundMusic.srcWav.fileStream,
                               this->backgroundMusic.srcWav.dataStartOffset + this->backgroundMusic.pos * 4, SEEK_SET)<0) {
#ifdef __3DS__
                        CtrRuntime::Log("Audio: loop seek failed");
#endif
                        SDL_RWclose(backgroundMusic.srcWav.fileStream);backgroundMusic.srcWav.fileStream=nullptr;
                        break;
                    }
                }
                else
                {
                    SDL_RWclose(this->backgroundMusic.srcWav.fileStream);
                    this->backgroundMusic.srcWav.fileStream = NULL;

                    break;
                }
            }
        }

        if (this->backgroundMusic.srcWav.fileStream && this->backgroundMusic.fadeoutLen != 0)
        {
            this->backgroundMusic.fadeoutProgress += samplesMixed;

            if (this->backgroundMusic.fadeoutProgress >= this->backgroundMusic.fadeoutLen)
            {
                SDL_RWclose(this->backgroundMusic.srcWav.fileStream);
                this->backgroundMusic.srcWav.fileStream = NULL;
            }
        }

        playingChannels++;
    }

    BgmMutex().unlock();

    // DirectSound supports playing from an arbitrary number of buffers at once, but that's kind of
    //   difficult to get right as it turns out. Instead we use 8 as an assumption of the
    //   max number of channels that could possibly be playing at once. If more channels end up in use,
    //   the input volume of each channel will start scaling down, which isn't correct, but would
    //   likely be imperceptible with that many channels anyway.

#ifndef __3DS__
    const int mixDivisor = std::max(8, (int)playingChannels);
#endif

    for (u32 i = 0; i < samples; i++)
    {
        // Integer division like this doesn't get optimized at all by the compiler. If it becomes
        //   a problem, it could be a good idea to convert to float, or to do the division as
        //   fixed point multiplication by the inverse of mixDivisor, depending on what's faster
        //   on any particular platform
#ifdef __3DS__
        // Keep original PCM amplitude; saturate summed effects rather than
        // attenuating even a lone music stream by 18 dB.
        finalBuffer[i] = std::clamp<i32>(mixBuffer[i], -32768, 32767);
#else
        finalBuffer[i] = mixBuffer[i] / mixDivisor;
#endif
    }

    SDL_QueueAudio(this->audioDev, finalBuffer.data(), samples * 2);
}

// EoSD originally just used this function to manage the streaming of the music WAV file.
//   We also use it to mix and queue audio, since we have to do that manually and doing it
//   in a thread keeps sound running continuously, even if the main thread runs into lag
void SoundPlayer::BackgroundMusicPlayerThread()
{
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    SDL_PauseAudioDevice(this->audioDev, 0);
    while (!terminateFlag)
    {
        // Queue depth follows actual consumption, avoiding wall-clock drift and
        // the old ever-growing latency limit after a slow loading frame.
#ifdef __3DS__
        if(SDL_GetQueuedAudioSize(audioDev)<CtrAudio::QueueBytes) MixAudio(CtrAudio::MixSamples);
#else
        if(SDL_GetQueuedAudioSize(audioDev)<12288) MixAudio(2048);
#endif
        else SDL_Delay(2);
    }
}

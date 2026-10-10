#pragma once
#include <SDL2/SDL_rwops.h>
// Exposes decoded stereo signed-16 PCM, with byte seeks for original sample loop points.
SDL_RWops* OpenVorbisPCM(const char* wavPath);
void PrepareStageMusic(const char paths[4][128]);

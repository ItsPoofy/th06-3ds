#pragma once
#include <SDL2/SDL_mutex.h>
class CtrMutex {
    SDL_mutex* value=nullptr;
public:
    CtrMutex() { value=SDL_CreateMutex(); }
    ~CtrMutex() { if(value) SDL_DestroyMutex(value); }
    void lock() { SDL_LockMutex(value); }
    void unlock() { SDL_UnlockMutex(value); }
};

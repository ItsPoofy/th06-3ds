#include "Runtime.hpp"
#include "GameLoop.hpp"
#include "DisplaySettings.hpp"
#include "Settings.hpp"
#include "Touch.hpp"
#include <3ds.h>
#include <SDL2/SDL.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <malloc.h>
namespace {
aptHookCookie lifecycleHook;
void OnLifecycle(APT_HookType type,void*) {
    if(type==APTHOOK_ONRESTORE || type==APTHOOK_ONWAKEUP)
        CtrGameLoop::RequestClockReset();
}
}
extern "C" {
    u32 __ctru_heap_size=32*1024*1024;
    u32 __ctru_linear_heap_size=24*1024*1024;
    u32 __stacksize__=512*1024;
}
void CtrRuntime::Log(const char* text) {
    FILE* f=fopen("sdmc:/3ds/th06/startup.log","a");
    if(f) {fprintf(f,"%s\n",text);fclose(f);}
    svcOutputDebugString(text,strlen(text));
}
void CtrRuntime::LoadingCheckpoint(const char* operation,int stage) {
    char message[192];const auto heap=mallinfo();
    snprintf(message,sizeof(message),"Stage %d: %s; heap used %d KiB, linear free %lu KiB",
        stage,operation,heap.uordblks/1024,static_cast<unsigned long>(linearSpaceFree()/1024));
    Log(message);
}
bool CtrRuntime::Initialize() {
    osSetSpeedupEnable(true);
    mkdir("sdmc:/3ds",0755);
    mkdir("sdmc:/3ds/th06",0755);
    if(chdir("sdmc:/3ds/th06")) return false;
    FILE* f=fopen("startup.log","w");if(f)fclose(f);
    f=fopen("sdmc:/3ds/th06/assets/asset-version.txt","rb");
    if(!f) {
        gfxInitDefault();
        consoleInit(GFX_BOTTOM,nullptr);
        printf("Touhou 6 for the 3DS\n\nAssets are missing.\n\nRun TH06 Asset Converter on your PC.\nCopy its assets folder to:\n\n sd:/3ds/th06/assets/\n\nPress START to exit.\n");
        while(aptMainLoop()) {
            hidScanInput();
            if(hidKeysDown()&KEY_START)break;
            gspWaitForVBlank();
        }
        gfxExit();
        return false;
    }
    fclose(f);
    CtrSettings::Load();CtrDisplay::Load();
    Log("Runtime initialized");
    if(SDL_Init(SDL_INIT_TIMER)) return false;
    aptHook(&lifecycleHook,OnLifecycle,nullptr);
    return true;
}
bool CtrRuntime::Poll() {
    if(!aptMainLoop()) return false;
    hidScanInput();
    CtrTouch::Scan();
    return (hidKeysHeld()&(KEY_START|KEY_SELECT))!=(KEY_START|KEY_SELECT);
}

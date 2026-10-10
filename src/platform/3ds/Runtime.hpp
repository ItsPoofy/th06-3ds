#pragma once
namespace CtrRuntime {
    void Log(const char* text);
    void LoadingCheckpoint(const char* operation,int stage);
    bool Initialize();
    bool Poll();
}

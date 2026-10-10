#pragma once
struct AnmVm;
namespace CtrGameplayLogo {
bool Initialize();
void Shutdown();
void Capture(const AnmVm* vms);
void Draw();
}

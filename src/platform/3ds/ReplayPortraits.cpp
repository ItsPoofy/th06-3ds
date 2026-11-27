#include "ReplayPortraits.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
namespace {CtrAssets::PackedTexture portraits[2];}
bool CtrReplayPortraits::Prepare() {
    return (portraits[0].ready || portraits[0].Load(TH06_ASSET_ROOT "ui/replay-reimu.bin")) &&
           (portraits[1].ready || portraits[1].Load(TH06_ASSET_ROOT "ui/replay-marisa.bin"));
}
void CtrReplayPortraits::Draw(int character,unsigned opacity) {
    if(character<0 || character>1 || !opacity)return;
    auto& portrait=portraits[character];if(!portrait.ready)return;
    CtrNative::DrawImage({&portrait.gpu,&portrait.sub},400-portrait.sub.width,0,
        C2D_Color32(255,255,255,opacity));
}
void CtrReplayPortraits::Release() {
    if((portraits[0].ready || portraits[1].ready) && g_GfxBackend)
        static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    for(auto& portrait:portraits)portrait.Clear();
}

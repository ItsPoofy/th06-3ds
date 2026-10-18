#include "CharacterPortraits.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
namespace {CtrAssets::PackedTexture portraits[2];}
bool CtrCharacterPortraits::Initialize() {
    return portraits[0].Load(TH06_ASSET_ROOT "ui/selection-reimu.bin") &&
           portraits[1].Load(TH06_ASSET_ROOT "ui/selection-marisa.bin");
}
void CtrCharacterPortraits::Shutdown() {for(auto& portrait:portraits)portrait.Clear();}
void CtrCharacterPortraits::Draw(int character,float slide,unsigned opacity) {
    auto& portrait=portraits[character];if(!portrait.ready || !opacity)return;
    // The fitted foreground and enlarged left shadow share one native panel.
    CtrNative::DrawImage({&portrait.gpu,&portrait.sub},character*160.f+slide,0,
        C2D_Color32(255,255,255,opacity));
}

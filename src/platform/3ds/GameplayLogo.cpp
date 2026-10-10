#include "GameplayLogo.hpp"
#include "AnmVm.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
namespace {
CtrAssets::PackedTexture atlas;
struct Part {Tex3DS_SubTexture sub{};float x=0,y=0,sx=1,sy=1,angle=0;unsigned color=0;};
Part parts[6];
}
bool CtrGameplayLogo::Initialize() {return atlas.Load("sdmc:/3ds/th06/assets/ui/gameplay-logo-atlas.bin");}
void CtrGameplayLogo::Shutdown() {atlas.Clear();for(auto& part:parts)part={};}
void CtrGameplayLogo::Capture(const AnmVm* vms) {
    constexpr int order[]={5,0,1,3,4,2};
    for(int i=0;i<6;i++) {
        const auto& vm=vms[order[i]];auto& part=parts[i];part.color=0;
        if(!atlas.ready || !vm.sprite || !vm.flags.isVisible || !vm.flags.flag1)continue;
        const auto& sprite=*vm.sprite;
        part.sub={u16(sprite.widthPx),u16(sprite.heightPx),
            sprite.startPixelInclusive.x/atlas.gpu.width,1-sprite.startPixelInclusive.y/atlas.gpu.height,
            sprite.endPixelInclusive.x/atlas.gpu.width,1-sprite.endPixelInclusive.y/atlas.gpu.height};
        constexpr float scale=1.2f;
        part.x=160+(vm.pos.x-528)*scale;part.y=120+(vm.pos.y-376)*scale;
        part.sx=vm.scaleX*scale;part.sy=vm.scaleY*scale;part.angle=vm.rotation.z;
        const unsigned a=(vm.color>>24)*88/255;
        part.color=C2D_Color32((vm.color>>16)&255,(vm.color>>8)&255,vm.color&255,a);
    }
}
void CtrGameplayLogo::Draw() {
    if(!atlas.ready)return;
    for(const auto& part:parts)if(part.color>>24)
        CtrNative::DrawImageRotated({&atlas.gpu,&part.sub},part.x,part.y,part.angle,part.color,part.sx,part.sy);
}

#pragma once
#include <cstddef>

namespace CtrCollision {
struct BombBounds {
    struct Rect {float left,top,right,bottom;};
    Rect active[16];
    unsigned count;
    template<class Projectile,std::size_t N> void Prepare(const Projectile (&projectiles)[N]) {
        static_assert(N<=16);
        count=0;
        for(const auto& p:projectiles)if(p.sizeX!=0) {
            active[count++]={p.posX-p.sizeX/2.f,p.posY-p.sizeY/2.f,
                p.posX+p.sizeX/2.f,p.posY+p.sizeY/2.f};
        }
    }
    bool Intersects(float left,float top,float right,float bottom) const {
        for(unsigned i=0;i<count;i++) {
            const auto& bomb=active[i];
            if(!(bomb.left>right || bomb.right<left || bomb.top>bottom || bomb.bottom<top))return true;
        }
        return false;
    }
};
}

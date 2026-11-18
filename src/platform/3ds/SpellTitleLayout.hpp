#pragma once
#include <algorithm>
#include "BossSpellLayout.hpp"

namespace CtrSpellTitleLayout {
struct Placement {float x,y,edgeLeft,edgeRight;};
inline constexpr float PlayfieldTop=16,PlayfieldBottom=464;
inline constexpr float BarMargin=24,BarPadding=4;
inline Placement Place(bool player,bool modern,float scriptX,float scriptY,
                       float zoom,float width,float height,float scaleX,float scaleY,float titleLeft) {
    // The original layout leaves 24px beside the text and 28px above/below it.
    float x=player?56+(scriptX-192):392+(scriptX-256)-width*zoom*scaleX;
    if(modern) {
        x=player?32+(scriptX-192):std::max(titleLeft,
            416+(scriptX-256)-width*zoom*scaleX);
    }
    const float restTop=modern?16.f:PlayfieldTop+BarMargin+BarPadding;
    const float playerBottom=PlayfieldBottom-BarMargin-BarPadding;
    const float restCenter=player?playerBottom-height*scaleY*.5f:restTop+height*scaleY*.5f;
    // Both names appear at the player's native lower-screen position, with
    // the same threefold zoom/30-frame fade before their settling motion.
    const float center=player?344+(scriptY-344)*(restCenter-344)/(440-344):
        344+(scriptY-312)*(344-restCenter)/(312-40);
    const float top=center-height*zoom*scaleY*.5f;
    return {x,top,modern && !player?32.f:x-8*zoom,
        modern && !player?416+(scriptX-256):x+width*zoom*scaleX+8*zoom};
}
}

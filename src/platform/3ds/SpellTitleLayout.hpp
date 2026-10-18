#pragma once

namespace CtrSpellTitleLayout {
struct Placement {float x,y,edgeLeft,edgeRight;};
inline Placement Place(bool player,bool modern,float scriptX,float scriptY,
                       float zoom,float textStart,float width,float height,float size,float musicHeight=0) {
    // Restore the player text's authored x=64 anchor. The boss title remains
    // right aligned. Texture cropping must not relocate the script's anchor.
    const float x=player?scriptX-128*zoom+textStart*zoom:
        416+(scriptX-256)-width*zoom*size;
    const float restTop=modern?16.f:20.f;
    const float playerBottom=musicHeight>0?440-musicHeight-8:436;
    const float restCenter=player?playerBottom-height*size*.5f:restTop+height*size*.5f;
    // Both names appear at the player's native lower-screen position, with
    // the same threefold zoom/30-frame fade before their settling motion.
    const float center=player?344+(scriptY-344)*(restCenter-344)/(440-344):
        344+(scriptY-312)*(344-restCenter)/(312-40);
    const float top=center-height*zoom*size*.5f;
    return {x,top,player || modern?32.f:x-8,player?x+width*zoom*size+8:416.f};
}
}

#pragma once
#include "ZunMath.hpp"
#include <cstddef>

namespace CtrGraphics {
// Shared by the sprite batch and PICA's linear vertex arena.
struct SpriteVertex {
    ZunVec3 position;
    ZunVec2 textureUV;
    u32 diffuse;
};
static_assert(sizeof(SpriteVertex)==24);
static_assert(offsetof(SpriteVertex,textureUV)==12);
static_assert(offsetof(SpriteVertex,diffuse)==20);
}

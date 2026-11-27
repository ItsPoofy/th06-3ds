#pragma once
#include <cstdint>
namespace CtrTexture {
inline std::uint32_t TexelOffset(std::uint32_t x,std::uint32_t y,std::uint32_t width) {
    std::uint32_t morton=0;
    for(std::uint32_t bit=0;bit<3;bit++)morton|=((x>>bit)&1)<<(2*bit)|((y>>bit)&1)<<(2*bit+1);
    return ((y>>3)*(width>>3)+(x>>3))*64+morton;
}
}

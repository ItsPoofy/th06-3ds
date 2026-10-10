#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace CtrBullets {
// Stable buckets retain vanilla alpha ordering, including pool order within
// each group. The pool is inspected once; drawing traverses only live indices.
template<std::size_t Capacity> class DrawOrder {
public:
    template<class Bullet, class Classify> void Build(const Bullet (&pool)[Capacity], Classify classify) {
        counts.fill(0);
        for(std::size_t i=0;i<Capacity;i++) {
            if(!pool[i].state)continue;
            const int group=classify(pool[i]);
            if(group>=0)indices[group][counts[group]++]=static_cast<std::uint16_t>(i);
        }
    }
    template<class Draw> void Each(Draw draw) const {
        for(std::size_t group=0;group<4;group++)
            for(std::size_t i=0;i<counts[group];i++)draw(indices[group][i]);
    }
private:
    static_assert(Capacity<=65536);
    std::array<std::array<std::uint16_t,Capacity>,4> indices;
    std::array<std::size_t,4> counts{};
};
}

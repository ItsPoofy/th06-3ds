#pragma once
#include "EclManager.hpp"
#include <cstddef>
#include <vector>

namespace Practice::Th06Run {
struct Link {int from,to;const EclRawInstr* instruction;};
struct ActorStart {const EclTimelineInstr* instruction;const EclTimelineInstr* resume;int sub,time,dialogueTime;};
class EclEntry {
public:
    void Clear();
    bool Load(const EclManager& manager,std::size_t fileSize,int difficulty);
    int SpellSub(int spell) const;
    bool Actor(bool midboss,ActorStart& actor) const;
    bool Path(int root,int target,std::vector<Link>& path) const;
    bool Contains(int sub,const EclRawInstr* instruction) const;
private:
    struct Sub {const unsigned char* begin;const unsigned char* end;bool boss=false;int spell=-1;std::vector<Link> links;};
    std::vector<Sub> subs;
    std::vector<ActorStart> actors;
};
}

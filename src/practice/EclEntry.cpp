#include "EclEntry.hpp"
#include <algorithm>
#include <cstring>
#include <queue>

void Practice::Th06Run::EclEntry::Clear(){subs.clear();actors.clear();}
bool Practice::Th06Run::EclEntry::Load(const EclManager& manager,std::size_t fileSize,int difficulty) {
    Clear();
    const auto* base=reinterpret_cast<const unsigned char*>(manager.eclFile);
    if(!base || fileSize<16 || difficulty<0 || difficulty>4)return false;
    const int count=manager.eclFile->subCount;
    if(count<=0 || std::size_t(count)>(fileSize-16)/4)return false;
    subs.resize(count);
    for(int i=0;i<count;i++) {
        const std::size_t offset=manager.eclFile->subOffsets[i],end=i+1<count?std::size_t(manager.eclFile->subOffsets[i+1]):fileSize;
        if(offset>end || end>fileSize)return false;
        auto& sub=subs[i];sub.begin=base+offset;sub.end=base+end;
        for(auto* p=sub.begin;std::size_t(sub.end-p)>=12;) {
            const auto* instruction=reinterpret_cast<const EclRawInstr*>(p);const int length=instruction->offsetToNext;
            if(length<12 || std::size_t(length)>std::size_t(sub.end-p))break;
            if(instruction->skipForDifficulty&(1<<difficulty)) {
                const int op=instruction->opCode,arg=length>=16?int(instruction->args.setInt):-1;
                if(op==ECL_OPCODE_BOSSSET && arg>=0)sub.boss=true;
                if(op==ECL_OPCODE_SPELLCARDSTART && length>=16)sub.spell=instruction->args.spellcardStart.spellcardId;
                if(arg>=0 && arg<count && (op==35 || (op>=37 && op<=42) || op==108 || op==109 || op==114 || op==116))sub.links.push_back({i,arg,instruction});
            }
            p+=length;
        }
    }
    const std::size_t timelineOffset=manager.eclFile->timelineOffsets[0];
    if(timelineOffset>fileSize)return false;
    const auto* p=base+timelineOffset;const auto* end=base+fileSize;
    int lastDialogue=-1,lastMessage=-1;
    while(std::size_t(end-p)>=8) {
        const auto* instruction=reinterpret_cast<const EclTimelineInstr*>(p);const int length=instruction->size,sub=instruction->arg0,op=instruction->opCode;
        if(instruction->time<0)break;
        if(length<8 || std::size_t(length)>std::size_t(end-p))return false;
        if(op==8){lastDialogue=instruction->time;lastMessage=instruction->arg0;}
        if(op>=0 && op<8 && sub>=0 && sub<count && subs[sub].boss &&
           std::none_of(actors.begin(),actors.end(),[sub](const auto& a){return a.sub==sub;}))
        {
            const int message=actors.empty()?2:0;
            const bool preceding=lastMessage==message && lastDialogue>=0 && instruction->time-lastDialogue<=120;
            actors.push_back({instruction,reinterpret_cast<const EclTimelineInstr*>(p+length),sub,instruction->time,preceding?lastDialogue:int(instruction->time)});
        }
        p+=length;
    }
    for(auto& actor:actors) {
        // Intro interrupts follow the dialogue wait. Resume after the last one,
        // retaining the native boss wait and every later timeline event.
        const auto* pos=reinterpret_cast<const unsigned char*>(actor.resume);
        while(std::size_t(end-pos)>=8) {
            const auto* instruction=reinterpret_cast<const EclTimelineInstr*>(pos);const int length=instruction->size,op=instruction->opCode;
            if(instruction->time<0 || length<8 || std::size_t(length)>std::size_t(end-pos))break;
            if(instruction->time>actor.time+120 || op<8)break;
            if(op==10)actor.resume=reinterpret_cast<const EclTimelineInstr*>(pos+length);
            if(op==12)break;
            pos+=length;
        }
    }
    return true;
}
int Practice::Th06Run::EclEntry::SpellSub(int spell) const {
    for(int i=0;i<int(subs.size());i++)if(subs[i].spell==spell)return i;
    return -1;
}
bool Practice::Th06Run::EclEntry::Actor(bool midboss,ActorStart& actor) const {
    const int index=midboss?0:1;
    if(index>=int(actors.size()))return false;
    actor=actors[index];return true;
}
bool Practice::Th06Run::EclEntry::Path(int root,int target,std::vector<Link>& path) const {
    path.clear();
    if(root<0 || target<0 || root>=int(subs.size()) || target>=int(subs.size()))return false;
    std::vector<int> parent(subs.size(),-1);std::vector<Link> incoming(subs.size());
    std::queue<int> queue;queue.push(root);parent[root]=root;
    while(!queue.empty() && parent[target]<0) {
        const int current=queue.front();queue.pop();
        for(const auto& link:subs[current].links)if(parent[link.to]<0){parent[link.to]=current;incoming[link.to]=link;queue.push(link.to);}
    }
    if(parent[target]<0)return false;
    for(int sub=target;sub!=root;sub=parent[sub])path.push_back(incoming[sub]);
    std::reverse(path.begin(),path.end());return true;
}
bool Practice::Th06Run::EclEntry::Contains(int sub,const EclRawInstr* instruction) const {
    if(sub<0 || sub>=int(subs.size()))return false;
    const auto* p=reinterpret_cast<const unsigned char*>(instruction);const auto& entry=subs[sub];
    return p>=entry.begin && p<=entry.end && std::size_t(entry.end-p)>=12 &&
        instruction->offsetToNext>=12 && std::size_t(instruction->offsetToNext)<=std::size_t(entry.end-p);
}

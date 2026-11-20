#include "Session.hpp"
#include <algorithm>

namespace {
Practice::Session session;
}
Practice::Session& Practice::Current() {return session;}
void Practice::Reset() {session=Session{};}
const char* Practice::TargetName(Target target) {
    static constexpr const char* names[]={"Beginning","Stage Portion","Mid Boss","End Boss","Nonspell","Spellcard","Frame"};
    return names[int(target)];
}
bool Practice::Matches(const Entry& entry,Target target,int stage,int difficulty,int shotRoute) {
    if(entry.stage!=stage || !(entry.difficultyMask&(1<<difficulty)) || (entry.route>=0 && entry.route!=shotRoute))return false;
    switch(target) {
    case Target::Midboss:return entry.midboss;
    case Target::Boss:return !entry.midboss;
    case Target::Nonspell:return entry.spell<0;
    case Target::Spell:return entry.spell>=0;
    default:return false;
    }
}
std::vector<const Practice::Entry*> Practice::Select(const std::vector<Entry>& entries,const Session& session) {
    std::vector<const Entry*> result;
    const int route=session.options.shotRoute<0?session.shot:session.options.shotRoute;
    for(const auto& entry:entries)if(Matches(entry,session.target,session.stage,session.difficulty,route))result.push_back(&entry);
    std::stable_sort(result.begin(),result.end(),[](const auto* a,const auto* b){
        if(a->midboss!=b->midboss)return a->midboss;
        return a->order<b->order;
    });
    return result;
}

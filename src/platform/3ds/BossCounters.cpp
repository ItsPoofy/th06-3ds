#include "BossCounters.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
struct Sub {bool boss=false;int lives=-1,spells=0;std::vector<int> edges,calls,callbacks;};
struct Encounter {int root,phases;std::vector<bool> members;};
std::vector<Sub> subs;
std::vector<Encounter> encounters;
std::vector<int> forecasts;
std::vector<bool> visited;
int phaseRoot=-1,activeCardSub=-1;
int consumedCards=0;
const void* active=nullptr;
int encounter=-1,completed=0,nativeLives=0,spells=0;
bool sharedBoss=false;
template<class T>T Read(const unsigned char* data,std::size_t at) {T value;std::memcpy(&value,data+at,sizeof(value));return value;}
void Reach(int sub,std::vector<bool>& visited) {
    if(sub<0 || sub>=int(subs.size()) || visited[sub])return;
    visited[sub]=true;
    for(int next:subs[sub].edges)Reach(next,visited);
}
int PhaseSpells(int sub) {
    if(sub<0 || sub>=int(subs.size()) || visited[sub])return 0;
    const auto& entry=subs[sub];
    if(entry.lives>=0 && entry.lives!=nativeLives)return 0;
    visited[sub]=true;int called=0,following=0;
    for(int next:entry.calls)called=std::max(called,PhaseSpells(next));
    for(int next:entry.callbacks)following=std::max(following,PhaseSpells(next));
    visited[sub]=false;
    // A called pattern runs before its life/death callback. Those spells are
    // consecutive; alternatives within either group remain mutually exclusive.
    return entry.spells+called+following;
}
int Forecast(int sub) {
    if(sub<0 || sub>=int(subs.size()))return 0;
    if(forecasts[sub]>=0)return forecasts[sub];
    std::fill(visited.begin(),visited.end(),false);
    return forecasts[sub]=PhaseSpells(sub);
}
void ResetCards(int root) {
    consumedCards=0;
    std::fill(forecasts.begin(),forecasts.end(),-1);
    phaseRoot=root;activeCardSub=-1;
}
void UpdateSpells() {spells=std::max(0,Forecast(phaseRoot)-consumedCards);}
int Remaining() {
    if(!sharedBoss)return active?nativeLives+1:0;
    int total=0;
    for(int i=completed;i<int(encounters.size());i++)
        total+=i==encounter?nativeLives+1:encounters[i].phases;
    return total;
}
}
void CtrBossCounters::Load(const void* input,std::size_t size,int difficulty,int stage) {
    subs.clear();encounters.clear();forecasts.clear();visited.clear();phaseRoot=activeCardSub=-1;active=nullptr;encounter=-1;completed=nativeLives=spells=consumedCards=0;
    // Rumia, Meiling and Sakuya return as the main boss in these stages.
    // Other stages pair different characters with independent phase counts.
    sharedBoss=stage==0 || stage==1 || stage==3 || stage==5;
    if(!input || size<16 || difficulty<0 || difficulty>4)return;
    const auto* data=static_cast<const unsigned char*>(input);
    const int count=Read<unsigned short>(data,0);
    if(count==0 || 16+std::size_t(count)*4>size)return;
    subs.resize(count);forecasts.assign(count,-1);visited.assign(count,false);
    for(int i=0;i<count;i++) {
        std::size_t pos=Read<unsigned>(data,16+i*4);
        const std::size_t end=i+1<count?Read<unsigned>(data,20+i*4):size;
        if(end>size || pos>end)continue;
        auto& sub=subs[i];
        int deathCallback=-1,lifeThreshold=-1,timerThreshold=-1;
        while(pos+12<=end) {
            const int op=Read<unsigned short>(data,pos+4),length=Read<unsigned short>(data,pos+6);
            if(length<12 || pos+length>end)break;
            if(data[pos+9]&(1<<difficulty)) {
                const int arg=length>=16?Read<int>(data,pos+12):-1;
                if(op==101 && arg>=0)sub.boss=true;
                if(op==126)sub.lives=std::max(sub.lives,arg);
                if(op==93)++sub.spells;
                if(op==113)lifeThreshold=arg;
                if(op==115)timerThreshold=arg;
                if(op==108)deathCallback=arg;
                if(arg>=0 && arg<count) {
                    if(op==35 || (op>=37 && op<=42) || op==109)sub.calls.push_back(arg);
                    if((op==114 && lifeThreshold>0) || (op==116 && timerThreshold>0))sub.callbacks.push_back(arg);
                }
                if(op==35 || (op>=37 && op<=42) || op==108 || op==109 || op==114 || op==116)
                    if(arg>=0 && arg<count)sub.edges.push_back(arg);
            }
            pos+=length;
        }
        // Difficulty-specific overrides replace an earlier death callback.
        if(deathCallback>=0 && deathCallback<count)sub.callbacks.push_back(deathCallback);
        // Repeated calls to the same pattern are one graph edge, not extra
        // paths to traverse on every phase callback.
        std::sort(sub.edges.begin(),sub.edges.end());
        sub.edges.erase(std::unique(sub.edges.begin(),sub.edges.end()),sub.edges.end());
    }
    std::size_t pos=Read<unsigned>(data,4);
    while(pos+8<=size) {
        const int time=Read<short>(data,pos),root=Read<short>(data,pos+2);
        const int op=Read<short>(data,pos+4),length=Read<unsigned short>(data,pos+6);
        if(time<0 || length<8 || pos+length>size)break;
        if(op>=0 && op<8 && root>=0 && root<count && subs[root].boss &&
           std::none_of(encounters.begin(),encounters.end(),[&](const auto& entry){return entry.root==root;})) {
            Encounter entry{root,1,std::vector<bool>(count)};Reach(root,entry.members);
            for(int i=0;i<count;i++)if(entry.members[i])entry.phases=std::max(entry.phases,subs[i].lives+1);
            encounters.push_back(std::move(entry));
        }
        pos+=length;
    }
}
void CtrBossCounters::Begin(const void* actor,int sub) {
    if(active==actor)return;
    for(int i=completed;i<int(encounters.size());i++)if(sub>=0 && sub<int(subs.size()) && encounters[i].members[sub]) {
        completed=i;encounter=i;active=actor;nativeLives=encounters[i].phases-1;
        ResetCards(sub);UpdateSpells();return;
    }
}
void CtrBossCounters::SetLives(const void* actor,int lives,int sub) {
    if(actor!=active || lives<0)return;
    if(lives!=nativeLives || phaseRoot<0)ResetCards(sub);
    nativeLives=lives;
    UpdateSpells();
}
void CtrBossCounters::SetCallback(const void* actor,int sub) {
    if(actor!=active)return;
    UpdateSpells();
}
void CtrBossCounters::StartSpell(const void* actor,int sub) {
    if(actor!=active)return;
    // Keep the phase's root when entering a called pattern; its callbacks
    // still contain the remaining spells in this same life.
    if(sub>=0 && sub<int(subs.size())) {
        if(activeCardSub==sub)return;
        activeCardSub=sub;
        ++consumedCards;
        UpdateSpells();
    }
}
void CtrBossCounters::EndSpell(const void* actor) {
    if(actor!=active)return;
    if(activeCardSub>=0) {
        activeCardSub=-1;
        UpdateSpells();
    }
}
void CtrBossCounters::Observe(bool present) {
    if(!present && active) {completed=encounter+1;encounter=-1;active=nullptr;spells=0;}
}
int CtrBossCounters::Lives() {return std::max(0,Remaining());}
int CtrBossCounters::Spells() {return spells;}

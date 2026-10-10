#include "BossPresentation.hpp"
#include "Enemy.hpp"
#include "Effect.hpp"
#include <array>
namespace {
const Enemy* actor=nullptr;
const Enemy* owner=nullptr;
struct Explosion {
    const Effect* effect=nullptr;unsigned generation=0;
    bool Playing()const{return effect && effect->inUseFlag && effect->spawnGeneration==generation;}
};
std::array<Explosion,8> explosions{};
}
void CtrBossPresentation::Reset(){actor=owner=nullptr;explosions={};}
void CtrBossPresentation::Begin(const Enemy* next) {
    if(next==actor)return;
    actor=owner=next;explosions={};
}
void CtrBossPresentation::TrackDefeat(const Enemy* enemy,const Effect* effect) {
    if(enemy!=owner || !effect || !effect->inUseFlag)return;
    for(auto& entry:explosions)if(!entry.Playing()) {entry={effect,effect->spawnGeneration};return;}
}
void CtrBossPresentation::Retire(const Enemy* enemy) {
    if(actor==enemy)actor=nullptr;
    // Enemy slots are reused by ordinary waves. Keep only already-started
    // defeat effects, never accept new effects from a retired actor's slot.
    if(owner==enemy)owner=nullptr;
}
bool CtrBossPresentation::Visible() {
    if(actor && actor->flags.active)return true;
    for(const auto& entry:explosions)if(entry.Playing())return true;
    return false;
}

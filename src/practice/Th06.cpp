#include "Th06.hpp"
#include "Th06Progress.hpp"
#include "GameManager.hpp"
#include "Th06Catalog.hpp"
#include "Th06CatalogJa.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>

const std::vector<Practice::Entry>& Practice::Th06::Entries() {
    static const std::vector<Entry> entries(std::begin(Catalog),std::end(Catalog));
    return entries;
}
const Practice::Stage& Practice::Th06::GetStage(int stage) {
    static constexpr Stage stages[]={
        {1,"Misty Lake",4,2},{2,"Lake shore",2,2},{3,"Mansion gate",4,3},
        {4,"Library",4,5},{5,"Mansion interior",3,2},{6,"Scarlet moon",2,0},{7,"Extra",4,3}
    };
    return stages[std::clamp(stage,1,7)-1];
}
const Practice::Entry* Practice::Th06::Selected() {
    const auto& session=Current();
    for(const auto* entry:Select(Entries(),session))if(entry->id==session.entry)return entry;
    return nullptr;
}
bool Practice::Th06::Encountered(const Entry& entry) {
    return entry.spell<0 || SpellKnown(entry.spell);
}
bool Practice::Th06::SpellKnown(int spell) {
    return spell>=0 && spell<CATK_NUM_CAPTURES &&
        (g_GameManager.catk[spell].numAttempts>0 || Th06Progress::SpellKnown(spell));
}
const char* Practice::Th06::SpellName(int spell,bool japanese) {
    for(const auto& entry:Entries())if(entry.spell==spell)return EntryName(entry,japanese);
    return "????????";
}
bool Practice::Th06::StageAvailable(int stage) {
    const auto& session=Current();
    if(session.Custom()) {
        const int difficulty=session.difficulty;
        if(difficulty<0 || difficulty>EXTRA || stage<1 || stage>(difficulty==EASY?5:7) ||
           (difficulty==EXTRA)!=(stage==7))return false;
        if(Th06Progress::StageVisited(difficulty,stage))return true;
        const auto& clears=g_GameManager.clrd[session.shot];
        const int reached=std::max(clears.difficultyClearedWithRetries[difficulty],clears.difficultyClearedWithoutRetries[difficulty]);
        if(difficulty==EXTRA)return reached>=6;
        if(reached>1 && stage<=std::min(reached+1,difficulty==EASY?5:6))return true;
        return stage<=6 && g_GameManager.pscr[session.shot][stage-1][difficulty].score>0;
    }
    if(stage==7)return g_GameManager.HasReachedMaxClears(g_GameManager.character,g_GameManager.shotType);
    const int count=std::clamp(int(g_GameManager.clrd[session.shot].difficultyClearedWithoutRetries[session.difficulty]),1,session.difficulty==EASY?5:6);
    return stage>=1 && stage<=count;
}
void Practice::Th06::Normalize() {
    auto& session=Current();const auto entries=Select(Entries(),session);
    const auto* current=Selected();
    if(!current || !Encountered(*current)) {
        session.entry.clear();
        for(const auto* entry:entries)if(Encountered(*entry)){session.entry=entry->id;break;}
        session.phase=0;
    }
    const auto* entry=Selected();
    if(!entry || session.phase>=int(entry->phases.size()))session.phase=0;
    const auto& stage=GetStage(session.stage);
    session.portion=std::clamp(session.portion,1,stage.firstPortions+stage.secondPortions);
}
int Practice::Th06::PortionFrame(int stage,int portion) {
    static constexpr int frames[7][9]={
        {68,580,1160,1540,2348,4438},{270,924,3528,4563},{340,1050,1670,2762,3807,4118,5274},
        {380,1454,2328,3392,4872,5712,7434,8354,9784},{350,1352,2292,3814,6774},
        {380,1484},{380,1300,2600,3680,4803,5933,7733}
    };
    return frames[stage-1][portion-1];
}
Practice::Setting Practice::Th06::StartSetting(int index,const Options& options) {
    static constexpr Setting settings[]={
        {"Lives",0,8,1},{"Bombs",0,8,1},{"Power",0,128,1},{"Score",0,999999990LL,10},
        {"Graze",0,99999,1},{"Point items",0,9999,1},{"Rank",0,32,1}
    };
    auto result=settings[index];
    if(index==6 && options.rankLock)result.maximum=99;
    return result;
}
std::int64_t Practice::Th06::Value(int index,const Options& options) {
    switch(index) {
    case 0:return options.lives;case 1:return options.bombs;case 2:return options.power;case 3:return options.score;
    case 4:return options.graze;case 5:return options.points;default:return options.rank;
    }
}
void Practice::Th06::SetValue(int index,std::int64_t value,Options& options) {
    const auto setting=StartSetting(index,options);
    value=std::clamp(value,setting.minimum,setting.maximum);value-=value%setting.step;
    switch(index) {
    case 0:options.lives=value;break;case 1:options.bombs=value;break;case 2:options.power=value;break;
    case 3:options.score=value;break;case 4:options.graze=value;break;case 5:options.points=value;break;
    case 6:options.rank=value;break;
    }
}

const char* Practice::Th06::EntryName(const Entry& entry,bool japanese) {
    const auto& entries=Entries();const auto index=&entry-entries.data();
    if(!japanese || index<0 || index>=std::ptrdiff_t(std::size(CatalogJa)))return entry.name.c_str();
    return CatalogJa[index];
}
const char* Practice::Th06::PhaseName(const Entry& entry,int phase,bool japanese) {
    if(phase<0 || phase>=int(entry.phases.size()))return "";
    if(japanese)for(const auto& names:CatalogPhasesJa)
        if(names.entry==&entry-Entries().data() && phase<3)return names.names[phase];
    return entry.phases[phase].c_str();
}
const char* Practice::Th06::StageName(int stage,bool japanese) {
    static constexpr const char* names[]={"霧の湖","湖畔","紅魔館 門前","大図書館","紅魔館 館内","紅い月","エクストラ"};
    return japanese?names[std::clamp(stage,1,7)-1]:GetStage(stage).name;
}

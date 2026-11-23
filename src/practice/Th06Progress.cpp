#include "Th06Progress.hpp"
#include <cstdint>
#include <cstdio>

namespace {
constexpr std::uint32_t Magic=0x31475054; // TPG1
struct Records {
    std::uint32_t magic=Magic;
    std::uint32_t stages[5]{};
    std::uint32_t spells[2]{};
} records;
static_assert(sizeof(Records)==32);
bool loaded=false;
constexpr const char* Path="practice-progress.dat";
void Load() {
    if(loaded)return;
    loaded=true;
    if(FILE* file=std::fopen(Path,"rb")) {
        Records saved;
        bool valid=std::fread(&saved,sizeof(saved),1,file)==1 && std::fgetc(file)==EOF && saved.magic==Magic;
        for(auto mask:saved.stages)valid=valid && !(mask&~0x7fu);
        std::fclose(file);
        if(valid)records=saved;
    }
}
bool Save() {
    FILE* file=std::fopen(Path,"wb");
    if(!file)return false;
    const bool written=std::fwrite(&records,sizeof(records),1,file)==1;
    const bool closed=std::fclose(file)==0;
    return written && closed;
}
bool ValidStage(int difficulty,int stage){return difficulty>=0 && difficulty<5 && stage>=1 && stage<=7;}
}
bool Practice::Th06Progress::StageVisited(int difficulty,int stage) {
    if(!ValidStage(difficulty,stage))return false;
    Load();return records.stages[difficulty]&(1u<<(stage-1));
}
bool Practice::Th06Progress::VisitStage(int difficulty,int stage) {
    if(!ValidStage(difficulty,stage))return false;
    Load();const auto bit=1u<<(stage-1);
    if(records.stages[difficulty]&bit)return true;
    records.stages[difficulty]|=bit;return Save();
}
bool Practice::Th06Progress::SpellKnown(int spell) {
    if(spell<0 || spell>=64)return false;
    Load();return records.spells[spell/32]&(1u<<(spell%32));
}
bool Practice::Th06Progress::RevealSpell(int spell) {
    if(spell<0 || spell>=64)return false;
    Load();const auto bit=1u<<(spell%32);
    if(records.spells[spell/32]&bit)return true;
    records.spells[spell/32]|=bit;return Save();
}

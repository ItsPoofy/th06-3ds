#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Practice {
enum class Mode { None, Regular, Custom };
enum class Target { Beginning, Portion, Midboss, Boss, Nonspell, Spell, Frame };
struct Options {
    int lives=8, bombs=8, power=128, graze=0, points=0, rank=32;
    std::int64_t score=0;
    bool rankLock=false,skipDialogue=true,repeat=false,stopAfterSection=true;
    int shotRoute=-1;
};
struct Entry {
    std::string id,name;
    int stage=1,route=-1,difficultyMask=0,order=0,spell=-1;
    bool midboss=false,dialogue=false;
    std::vector<std::string> phases;
};
struct Stage {
    int id;
    const char* name;
    int firstPortions,secondPortions;
};
struct Setting {
    const char* name;
    std::int64_t minimum,maximum,step;
};
struct Session {
    Mode mode=Mode::None;
    Target target=Target::Beginning;
    Options options;
    int stage=1,difficulty=1,shot=0,portion=1,frame=0,phase=0;
    std::string entry;
    bool running=false;
    bool reopen=false;
    Mode previous=Mode::None;
    bool Custom() const {return mode==Mode::Custom;}
    bool KeepsMusicOnRetry() const {return Custom() && options.stopAfterSection;}
    bool RetriesDeath() const {return Custom() && options.repeat;}
    bool SavesPoints() const {return mode!=Mode::Custom;}
    bool SavesHistory() const {return mode!=Mode::Custom;}
};
Session& Current();
void Reset();
const char* TargetName(Target target);
bool Matches(const Entry& entry,Target target,int stage,int difficulty,int shotRoute);
std::vector<const Entry*> Select(const std::vector<Entry>& entries,const Session& session);

}

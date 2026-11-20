#pragma once
#include "Session.hpp"
namespace Practice::Th06 {
Setting StartSetting(int index,const Options& options);
std::int64_t Value(int index,const Options& options);
void SetValue(int index,std::int64_t value,Options& options);
const std::vector<Entry>& Entries();
const Stage& GetStage(int stage);
const Entry* Selected();
bool Encountered(const Entry& entry);
bool StageAvailable(int stage);
void Normalize();
int PortionFrame(int stage,int portion);
const char* EntryName(const Entry& entry,bool japanese);
const char* PhaseName(const Entry& entry,int phase,bool japanese);
const char* StageName(int stage,bool japanese);
}

#pragma once
#include <cstdint>
struct ScoreRaw;
namespace CtrStageScores {
inline constexpr int TotalCount=5;
struct TotalScore {std::uint32_t score;char name[9];};
const TotalScore& Total(int characterShot,int difficulty,int rank);
void Load(const ScoreRaw* savedRuns);
void BeginStage();
void RecordCurrent();
std::uint32_t Get(int characterShot,int difficulty,int stage);
}

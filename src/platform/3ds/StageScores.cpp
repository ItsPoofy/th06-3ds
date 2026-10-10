#include "StageScores.hpp"
#include "GameManager.hpp"
#include "ResultScreen.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace {
struct Records {
    std::uint32_t magic=0x32475453; // STG2: points earned within each individual stage
    std::uint32_t scores[4][5][7]{};
} records;
bool loaded=false,dirty=false;
std::uint32_t stageStartScore=0;
CtrStageScores::TotalScore totals[4][5][CtrStageScores::TotalCount]{};
void TotalRecord(const Hscr& run) {
    if(run.character>=4 || run.difficulty>=5)return;
    auto& rows=totals[run.character][run.difficulty];
    for(int rank=0;rank<CtrStageScores::TotalCount;rank++)if(run.score>=rows[rank].score) {
        for(int i=CtrStageScores::TotalCount-1;i>rank;i--)rows[i]=rows[i-1];
        rows[rank].score=std::min<std::uint32_t>(run.score,999999999u);
        std::memcpy(rows[rank].name,run.name,8);rows[rank].name[8]=0;return;
    }
}
void Record(int shot,int difficulty,int stage,std::uint32_t score) {
    if(shot<0 || shot>=4 || difficulty<0 || difficulty>=5 || stage<1 || stage>7)return;
    auto& best=records.scores[shot][difficulty][stage-1];
    score=std::min<std::uint32_t>(score,999999999u);
    if(score>best){best=score;dirty=true;}
}
void Save() {
    if(!dirty)return;
    FILE* file=std::fopen("stage-scores.dat","wb");if(!file)return;
    const bool written=std::fwrite(&records,sizeof(records),1,file)==1;
    const bool closed=std::fclose(file)==0;
    if(written && closed)dirty=false;
}
}
void CtrStageScores::Load(const ScoreRaw* savedRuns) {
    records=Records{};loaded=true;dirty=false;
    // Include the same defaults as the native post-game scoreboard.
    for(auto& shot:totals)for(auto& difficulty:shot)for(int rank=0;rank<TotalCount;rank++)
        difficulty[rank]=TotalScore{std::uint32_t(1000000-rank*100000),"Nanashi "};
    if(FILE* file=std::fopen("stage-scores.dat","rb")) {
        Records saved;
        if(std::fread(&saved,sizeof(saved),1,file)==1) {
            if(saved.magic==records.magic)records=saved;
            else if(saved.magic==0x31475453) {
                // Older records were cumulative. Only stage one and Extra
                // already represent a whole individual stage.
                for(int shot=0;shot<4;shot++)for(int d=0;d<5;d++) {
                    records.scores[shot][d][0]=saved.scores[shot][d][0];
                    records.scores[shot][d][6]=saved.scores[shot][d][6];
                }
                dirty=true;
            }
        }
        std::fclose(file);
    }
    // Native run records provide totals; practice records provide stage scores.
    // Later-stage cumulative run totals cannot reconstruct individual stages.
    if(savedRuns && savedRuns->dataOffset<=savedRuns->fileLen) {
        const auto* bytes=reinterpret_cast<const unsigned char*>(savedRuns);
        std::uint32_t offset=savedRuns->dataOffset;
        while(savedRuns->fileLen-offset>=sizeof(Th6k)) {
            Th6k header;std::memcpy(&header,bytes+offset,sizeof(header));
            if(header.th6kLen<sizeof(Th6k) || header.th6kLen>savedRuns->fileLen-offset)break;
            if(header.magic==HSCR_MAGIC && header.version==TH6K_VERSION && header.th6kLen>=sizeof(Hscr)) {
                Hscr run;std::memcpy(&run,bytes+offset,sizeof(run));
                TotalRecord(run);
                const int stage=run.stage==99?(run.difficulty==EXTRA?7:run.difficulty==EASY?5:6):run.stage;
                if(stage==1 || run.difficulty==EXTRA)Record(run.character,run.difficulty,stage,run.score);
            }
            if(header.magic==PSCR_MAGIC && header.version==TH6K_VERSION && header.th6kLen>=sizeof(Pscr)) {
                Pscr practice;std::memcpy(&practice,bytes+offset,sizeof(practice));
                if(practice.score>0 && practice.character<4 && practice.difficulty<4 && practice.stage<6)
                    Record(practice.character,practice.difficulty,practice.stage+1,practice.score);
            }
            offset+=header.th6kLen;
        }
    }
    Save();
}
void CtrStageScores::BeginStage() {
    stageStartScore=std::max(0,int(g_GameManager.guiScore));
}
void CtrStageScores::RecordCurrent() {
    if(g_GameManager.isInReplay || g_GameManager.demoMode || g_GameManager.isInPracticeMode)return;
    if(!loaded)Load(nullptr);
    const auto total=std::uint32_t(std::max({0,int(g_GameManager.score),int(g_GameManager.guiScore)}));
    Record(g_GameManager.CharacterShotType(),g_GameManager.difficulty,g_GameManager.currentStage,total>=stageStartScore?total-stageStartScore:0);
    Save();
}
std::uint32_t CtrStageScores::Get(int shot,int difficulty,int stage) {
    if(shot<0 || shot>=4 || difficulty<0 || difficulty>=5 || stage<0 || stage>=7)return 0;
    return records.scores[shot][difficulty][stage];
}

const CtrStageScores::TotalScore& CtrStageScores::Total(int shot,int difficulty,int rank) {
    static constexpr TotalScore empty{0,"--------"};
    if(shot<0 || shot>=4 || difficulty<0 || difficulty>=5 || rank<0 || rank>=TotalCount)return empty;
    return totals[shot][difficulty][rank];
}

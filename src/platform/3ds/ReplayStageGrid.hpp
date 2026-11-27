#pragma once
#include <algorithm>
#include <array>
struct CtrReplayStageGrid {
    std::array<int,7> stages{};
    int count=0,columns=1,rows=0;
    explicit CtrReplayStageGrid(const std::array<bool,7>& present) {
        for(int stage=0;stage<7;stage++)if(present[stage])stages[count++]=stage;
        columns=count>3?2:1;rows=(count+columns-1)/columns;
    }
    int Move(int stage,int horizontal,int vertical) const {
        if(!count)return stage;
        int index=0;while(index<count && stages[index]!=stage)++index;
        if(index==count)return stages[0];
        if(vertical)index=(index+vertical+count)%count;
        else if(horizontal && columns>1) {
            const int column=(index/rows+columns+horizontal)%columns;
            index=column*rows+std::min(index%rows,count-column*rows-1);
        }
        return stages[index];
    }
};

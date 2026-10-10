#pragma once
struct Stage;
namespace CtrStageDepth {
void Prepare(const Stage& stage);
float Offset(unsigned instance);
void Clear();
}

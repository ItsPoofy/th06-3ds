#pragma once
namespace CtrDisplay {
enum class Alignment { Top, Middle, Bottom };
void Load();
void Cycle(int direction);
void Reset();
Alignment GetAlignment();
const char* AlignmentName();
float PlayfieldOffset();
}

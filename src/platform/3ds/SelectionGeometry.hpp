#pragma once
#include "AnmVm.hpp"
namespace CtrSelectionGeometry {
inline constexpr float DifficultyHeight=48.f,DifficultyStep=49.f,DifficultyTextScale=1.f;
inline constexpr float DifficultyY(int row,bool extra) {return extra?100.f:40.f+DifficultyStep*row;}
// Both native sprite pieces and return snapshots use the same transform.
inline constexpr float Scale(int character) {return character==0?1.04f:.92f;}
inline constexpr float Down(int character) {return character==0?14.f:8.f;}
inline float X(const AnmVm& vm,int character) {return 320+(vm.pos.x-448)*Scale(character);}
inline float Y(const AnmVm& vm,int character) {return Down(character)+vm.pos.y*Scale(character);}
}

#pragma once
#include "RecordsPanel.hpp"
struct ResultScreen;
namespace CtrRecords {
inline constexpr int VisibleRows=5;
void OpenList(ResultScreen& screen);
bool Input(ResultScreen& screen);
void DrawList(ResultScreen& screen,CtrRecordsPanel::View view);
void DrawDetails(ResultScreen& screen,CtrRecordsPanel::View view);
}

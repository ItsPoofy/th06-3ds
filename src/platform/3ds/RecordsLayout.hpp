#pragma once
#include "RecordsPanel.hpp"
struct ResultScreen;
// Upper-screen spell list with selected-card details below.
namespace CtrRecords {
inline constexpr int VisibleRows=11;
// Redraw names on open because other screens share their text canvases.
void OpenList(ResultScreen& screen);
// Up/Down select cards, Left/Right select pages. Returns true on B.
bool Input(ResultScreen& screen);
void DrawList(ResultScreen& screen,CtrRecordsPanel::View view);
void DrawDetails(ResultScreen& screen,CtrRecordsPanel::View view);
}

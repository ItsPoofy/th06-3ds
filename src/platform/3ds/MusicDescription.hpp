#pragma once
// Lays out a Music Room comment as wrapped rows on the lower screen.
namespace CtrMusicDescription {
// One text sprite holds part of a row. The second part starts where the first
// part's text ends, so a row reads as one line across both sprites.
struct Part {char text[128];float x;};
struct Row {Part parts[2];int partCount;};
// Joins the comment's source lines (Shift-JIS or ASCII, as in musiccmt.txt) into
// one paragraph and wraps it to rowWidth. Widths are text-sprite pixels measured
// at fontHeight. Returns the number of rows written, at most capacity.
int Layout(const char (*lines)[66],int lineCount,int fontHeight,float spriteWidth,float rowWidth,Row* rows,int capacity);
}

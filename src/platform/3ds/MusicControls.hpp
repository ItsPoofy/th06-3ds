#pragma once
// Music Room playback buttons, drawn as ASCII-font text on the lower screen.
namespace CtrMusicControls {
enum class Action {None,Previous,StartStop,Next};
// Queues the times and buttons for this frame's lower-screen ASCII pass.
void Draw();
// Maps a lower-screen tap to a button and highlights it briefly.
Action Hit(int x,int y);
}

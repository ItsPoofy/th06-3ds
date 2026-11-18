#pragma once
// Music Room playback controls, drawn as ASCII-font text on the lower screen.
namespace CtrMusicControls {
enum class Action {None,Previous,PlayPause,Next,Seek};
// Queues the times and buttons for this frame's lower-screen ASCII pass.
void Draw();
// Draws the scrubber gauge. Call while the renderer's menu layer is active.
void DrawGauge();
// Maps a lower-screen tap to a control and highlights it briefly. For Seek,
// fraction receives the tapped position along the scrubber (0 to 1).
Action Hit(int x,int y,float& fraction);
}

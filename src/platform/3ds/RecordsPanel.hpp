#pragma once
#include <citro2d.h>
namespace CtrRecordsPanel {
enum Row {DifficultyRow,CharacterRow,ShotRow,ButtonRow,RowCount};
enum Button {SpellCardsButton,QuitButton};
// The current choice. It stays set between visits to the records screen.
struct Board {int difficulty=0,character=0,shot=0,focus=DifficultyRow,button=SpellCardsButton;};
Board& Current();

// Layout of the lower screen in menu-layer coordinates (640x480, shown at half size).
inline constexpr float Left=56,Right=584,Margin=40,Section=100,Caption=16;
inline constexpr float SectionTop(int row) {return Margin+row*Section;}
inline constexpr float RowCenter(int row) {return SectionTop(row)+(Caption+Section)/2;}
inline constexpr float Divider=Margin+3*Section+4;
inline constexpr float ButtonCenter=(Divider+480-Margin)/2;

// Layout of the score table in scene coordinates (640x480, shown at half size).
inline constexpr float TableTop=152,TableSpacing=30,TableScale=1.25f;
inline constexpr float RankX=36,NameX=80,ScoreX=210,StageX=390;

void Show(bool controls);
void Clear();
void Release();
// The table backdrop, drawn under the scene.
void DrawBackdrop(C3D_RenderTarget* top);
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);

struct Hit {int row=-1,index=0;};
// Maps a lower-screen tap (screen pixels) to a row and choice.
Hit Touch(int x,int y);
}

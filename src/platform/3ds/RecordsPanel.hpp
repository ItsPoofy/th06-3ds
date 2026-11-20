#pragma once
#include <citro2d.h>
#include "MenuCanvas.hpp"
namespace CtrRecordsPanel {
enum Row {DifficultyRow,CharacterRow,ShotRow,ButtonRow,RowCount};
enum Button {SpellCardsButton,QuitButton};
// The current choice. It stays set between visits to the records screen.
struct Board {int difficulty=0,character=0,shot=0,focus=DifficultyRow,button=SpellCardsButton;};
Board& Current();

// Layout of the lower screen in menu-layer coordinates (640x480, shown at half size).
inline constexpr float Left=56,Right=584,Margin=40,Section=100,Caption=20,CaptionScale=1.25f;
inline constexpr float SectionTop(int row) {return Margin+row*Section;}
inline constexpr float RowCenter(int row) {return SectionTop(row)+(Caption+Section)/2;}
inline constexpr float Divider=Margin+3*Section+4;
inline constexpr float ButtonCenter=(Divider+480-Margin)/2;

// Layout of the score table in the 800x480 upper canvas, shown at half size.
inline constexpr float TableTop=168,TableSpacing=28,TableScale=1.5f,TableHeaderScale=CaptionScale;
inline constexpr float RankX=36,NameX=80,ScoreX=416,StageX=724;

// slide is in upper-canvas pixels. LowerSlide maps it to the 640 px lower
// canvas. Both views draw throughout a board/list transition.
struct View {
    float slide=0,opacity=1;
    float LowerSlide() const {return slide*640/CtrMenuCanvas::Width;}
};
void Show(bool controls,View view={});
void ShowSpellCards(View view={});
void ShowTitle(float y);
void Clear();
void Release();
void Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom);

struct Hit {int row=-1,index=0;};
// Maps a lower-screen tap (screen pixels) to a row and choice.
Hit Touch(int x,int y);
}

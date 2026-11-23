#include "PracticeReturn.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "CtrRenderer.hpp"
#include "GameWindow.hpp"
#include "GameplayLayout.hpp"
#include "LatchedInput.hpp"
#include "Touch.hpp"
#include "Settings.hpp"
#include "SoundPlayer.hpp"
#include "Controller.hpp"
#include "Supervisor.hpp"
#include "utils.hpp"
#include "practice/Completion.hpp"
#include "practice/RunTransition.hpp"
#include "../../../assets/practice_return.hpp"
#include <algorithm>

namespace {
using namespace Practice::Completion;
CtrAssets::PackedTexture atlas;
constexpr Action Actions[]={Action::Retry,Action::ReturnPractice,Action::Quit};
constexpr float RowTop=64,RowSpacing=40;
int cursor=0;
bool opened=false;
void Label(int index,float centerX,float centerY,float maxWidth,unsigned color) {
    const auto& label=CtrPracticeReturnAssets::labels[index];
    const float w=atlas.gpu.width,h=atlas.gpu.height;
    const float scale=std::min(1.f,maxWidth/label.w);
    Tex3DS_SubTexture sub{u16(label.w),u16(label.h),label.x/w,1-label.y/h,(label.x+label.w)/w,1-(label.y+label.h)/h};
    CtrNative::DrawImage({&atlas.gpu,&sub},centerX-label.w*scale*.5f,centerY-label.h*scale*.5f,color,scale,scale);
}
}
bool CtrPracticeReturn::Initialize() {
    opened=false;cursor=0;
    return atlas.ready || atlas.Load(TH06_ASSET_ROOT "ui/practice-return.bin");
}
void CtrPracticeReturn::Shutdown() {
    opened=false;
    if(atlas.ready && g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    atlas.Clear();
}
void CtrPracticeReturn::Update() {
    if(!CountingDown() || Leaving())return;
    if(!opened) {
        cursor=AutomaticAction()==Action::Retry?0:1;opened=true;
        return;
    }
    const int previous=cursor;
    if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP))cursor=(cursor+2)%3;
    if(WAS_PRESSED_PERIODIC(TH_BUTTON_DOWN))cursor=(cursor+1)%3;
    const auto pressed=CtrLatchedInput::Pressed(TH_BUTTON_ANY);
    bool accept=(pressed&TH_BUTTON_SELECTMENU)!=0;
    int x=0,y=0;
    if(CtrTouch::Tap(x,y) && x>=14 && x<306 && y>=RowTop && y<RowTop+3*RowSpacing) {
        const int row=int((y-RowTop)/RowSpacing);
        accept=row==cursor;cursor=row;
    }
    if(cursor!=previous)g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    if(pressed&TH_BUTTON_RETURNMENU) {
        Choose(Action::ReturnPractice);g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    } else if(accept) {
        Choose(Actions[cursor]);g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
    }
}
void CtrPracticeReturn::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    const bool showChoices=CountingDown() && atlas.ready;
    if(!showChoices)return;
    const unsigned opacity=unsigned(255*TextOpacity());
    const int language=CtrSettings::English()?0:1;
    const int action=AutomaticAction()==Action::Retry?1:0;
    C2D_ViewReset();C2D_SceneBegin(top);
    const float left=CtrLayout::Left(),width=384*CtrLayout::Scale();
    const float visibleTop=std::max(0.f,CtrLayout::Offset());
    const float centerY=(visibleTop+240)*.5f;
    C2D_DrawRectSolid(left,centerY-24,0,width,48,C2D_Color32(0,0,0,unsigned(150*TextOpacity())));
    Label((language*2+action)*CountdownSeconds+SecondsRemaining()-1,200,centerY,width-16,
        C2D_Color32(255,255,255,opacity));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0,0,0,320,240,C2D_Color32(0,0,0,unsigned(217*TextOpacity())));
    for(int row=0;row<3;row++) {
        const bool selected=row==cursor;
        const unsigned color=selected?C2D_Color32(255,255,128,opacity):C2D_Color32(255,255,255,opacity*3/4);
        Label(CtrPracticeReturnAssets::ChoiceStart+language*3+row,160,RowTop+(row+.5f)*RowSpacing,280,color);
        if(selected) {
            const float y=RowTop+(row+.5f)*RowSpacing;
            C2D_DrawTriangle(16,y-5,color,24,y,color,16,y+5,color,0);
        }
    }
}
void CtrPracticeReturn::DrawTransition(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!Practice::RunTransition::Active())return;
    const float transition=Practice::RunTransition::Opacity();
    if(transition>0) {
        const auto fade=C2D_Color32(0,0,0,unsigned(255*transition));
        C2D_ViewReset();
        C2D_SceneBegin(bottom);C2D_DrawRectSolid(0,0,0,320,240,fade);
        C2D_SceneBegin(top);C2D_DrawRectSolid(0,0,0,400,240,fade);
    }
    Practice::RunTransition::Presented();
}

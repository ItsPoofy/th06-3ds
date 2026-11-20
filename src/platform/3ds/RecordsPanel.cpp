#include "RecordsPanel.hpp"
#include "PackedTexture.hpp"
#include "NativeImage.hpp"
#include "ContentAssets.hpp"
#include "CtrRenderer.hpp"
#include "Presentation.hpp"
#include "Settings.hpp"
#include "GameWindow.hpp"
#include "../../../assets/records_labels.hpp"
#include <cstdio>
#include <cstring>

namespace {
CtrAssets::PackedTexture atlas;
bool atlasEnglish=false;
struct Shown {bool on=false;CtrRecordsPanel::View view;};
Shown boardShown,spellShown;
bool controlsShown=false;
bool titleShown=false;
float titleTop=0;
CtrRecordsPanel::Board board;

// Labels are drawn at half size: the atlas holds them at layer resolution.
constexpr float LabelScale=.5f;
// Distance from a lower-screen label's top to the middle of its letters, in layer pixels.
constexpr float InkCenter=25.5f;
constexpr unsigned Dimmed=135;

const CtrRecordsAssets::Label* Find(const char* name) {
    for(const auto& label:CtrRecordsAssets::labels)if(!std::strcmp(label.name,name))return &label;
    return nullptr;
}
int Width(const CtrRecordsAssets::Label& label) {return atlasEnglish?label.english_w:label.japanese_w;}
float LayerWidth(const char* name) {const auto* label=Find(name);return label?Width(*label)*1.f:0;}

bool Ready() {
    if(atlas.ready && atlasEnglish==CtrSettings::English())return true;
    if(atlas.ready && g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    atlasEnglish=CtrSettings::English();
    return atlas.Load(CtrContent::Path("ui","records-atlas.bin").c_str());
}

void Label(const char* name,float x,float y,unsigned alpha=255,float scale=LabelScale) {
    const auto* label=Find(name);
    if(!label)return;
    const int width=Width(*label);
    const float w=atlas.gpu.width,h=atlas.gpu.height;
    Tex3DS_SubTexture sub{u16(width),u16(label->h),label->x/w,1-label->y/h,(label->x+width)/w,1-(label->y+label->h)/h};
    CtrNative::DrawImage({&atlas.gpu,&sub},x,y,C2D_Color32(255,255,255,alpha),scale,scale);
}
void Centered(const char* name,float cx,float cy,bool selected,float opacity) {
    Label(name,(cx-LayerWidth(name)/2)*LabelScale,(cy-InkCenter)*LabelScale,unsigned((selected?255:Dimmed)*opacity));
}
void Line(float x0,float y,float x1,unsigned color) {C2D_DrawRectSolid(x0,y,0,x1-x0,1,color);}

void DifficultyEdges(float* left,float* width) {
    float total=0;
    for(int i=0;i<5;i++) {char name[16];std::snprintf(name,sizeof(name),"difficulty%d",i);total+=width[i]=LayerWidth(name);}
    const float gap=(CtrRecordsPanel::Right-CtrRecordsPanel::Left-total)/4;
    float x=CtrRecordsPanel::Left;
    for(int i=0;i<5;i++) {left[i]=x;x+=width[i]+gap;}
}
}

CtrRecordsPanel::Board& CtrRecordsPanel::Current() {return board;}
void CtrRecordsPanel::Show(bool controls,View view) {
    boardShown={true,view};controlsShown=controls;
    CtrPresentation::ShowListBox(24+view.slide,TableTop-34,CtrMenuCanvas::Width-24+view.slide,TableTop+10*TableSpacing+6,false,false,view.opacity);
}
void CtrRecordsPanel::ShowSpellCards(View view) {spellShown={true,view};}
void CtrRecordsPanel::ShowTitle(float y) {titleShown=true;titleTop=y*LabelScale;}
void CtrRecordsPanel::Clear() {boardShown.on=spellShown.on=titleShown=false;}
void CtrRecordsPanel::Release() {
    if(atlas.ready && g_GfxBackend)static_cast<CtrRenderer*>(g_GfxBackend)->SynchronizeAssets();
    atlas.Clear();
}

namespace {
constexpr float HeaderX=CtrRecordsPanel::RankX/2,HeaderY=51;
unsigned Rule(float opacity) {return C2D_Color32(120,60,60,unsigned(255*opacity));}
void Move(const CtrRecordsPanel::View& view,bool lower) {
    C2D_ViewReset();C2D_ViewTranslate((lower?view.LowerSlide():view.slide)*LabelScale,0);
}

void DrawSpellCards(C3D_RenderTarget* top,C3D_RenderTarget* bottom,CtrRecordsPanel::View view) {
    using namespace CtrRecordsPanel;
    const float opacity=view.opacity;
    C2D_SceneBegin(top);
    Move(view,false);
    // The atlas holds this label at the lower screen's size. Draw it at row size.
    Label("spellcards",HeaderX,HeaderY,unsigned(255*opacity),LabelScale*1.25f/1.5f);
    C2D_SceneBegin(bottom);
    Move(view,true);
    // The caption "SPELL CARD No.00" is 16 characters.
    Line((Left+14*16*CaptionScale+12)*LabelScale,(SectionTop(0)+Caption/2)*LabelScale,Right*LabelScale,Rule(opacity));
}

void DrawBoard(C3D_RenderTarget* top,C3D_RenderTarget* bottom,CtrRecordsPanel::View view) {
    using namespace CtrRecordsPanel;
    const float opacity=view.opacity;
    C2D_SceneBegin(top);
    Move(view,false);
    const unsigned alpha=unsigned(255*opacity);
    char name[24];
    std::snprintf(name,sizeof(name),"small-difficulty%d",board.difficulty);
    Label(name,HeaderX,HeaderY,alpha);
    const float difficultyWidth=LayerWidth(name);
    std::snprintf(name,sizeof(name),"header%d",board.character*2+board.shot);
    Label(name,HeaderX+(difficultyWidth+20)*LabelScale,HeaderY,alpha);
    Line(RankX/2,(TableTop-6)/2,(CtrMenuCanvas::Width-36)/2.f,C2D_Color32(255,192,192,unsigned(110*opacity)));
    if(!controlsShown)return;

    C2D_SceneBegin(bottom);
    Move(view,true);
    const unsigned rule=Rule(opacity);
    static const char* captions[]={"DIFFICULTY","CHARACTER","SHOT TYPE"};
    for(int row=0;row<3;row++) {
        const float y=(SectionTop(row)+Caption/2)*LabelScale;
        Line((Left+14*CaptionScale*std::strlen(captions[row])+12)*LabelScale,y,Right*LabelScale,rule);
    }
    Line(Left*LabelScale,Divider*LabelScale,Right*LabelScale,rule);

    float left[5],width[5];
    DifficultyEdges(left,width);
    for(int i=0;i<5;i++) {
        std::snprintf(name,sizeof(name),"difficulty%d",i);
        Centered(name,left[i]+width[i]/2,RowCenter(DifficultyRow),i==board.difficulty,opacity);
    }
    const float half=(Right-Left)/2;
    for(int i=0;i<2;i++) {
        std::snprintf(name,sizeof(name),"character%d",i);
        Centered(name,Left+half*(i+.5f),RowCenter(CharacterRow),i==board.character,opacity);
        std::snprintf(name,sizeof(name),"shot%d",board.character*2+i);
        Centered(name,Left+half*(i+.5f),RowCenter(ShotRow),i==board.shot,opacity);
    }
    const bool buttons=board.focus==ButtonRow;
    Centered("spellcards",Left+half*.5f,ButtonCenter,!buttons || board.button==SpellCardsButton,opacity);
    Centered("quit",Left+half*1.5f,ButtonCenter,!buttons || board.button==QuitButton,opacity);
}
}

void CtrRecordsPanel::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if((!boardShown.on && !spellShown.on && !titleShown) || !Ready())return;
    if(titleShown) {
        C2D_ViewReset();C2D_SceneBegin(top);
        Label("records-title",24,titleTop);
        Label("records-subtitle",24,titleTop+30);
    }
    if(boardShown.on)DrawBoard(top,bottom,boardShown.view);
    if(spellShown.on)DrawSpellCards(top,bottom,spellShown.view);
    C2D_ViewReset();
}

CtrRecordsPanel::Hit CtrRecordsPanel::Touch(int x,int y) {
    const float layerX=x/LabelScale,layerY=y/LabelScale;
    Hit hit;
    if(layerY>=Divider) {hit.row=ButtonRow;hit.index=layerX<320?SpellCardsButton:QuitButton;return hit;}
    for(int row=0;row<3;row++) {
        if(layerY<SectionTop(row) || layerY>=SectionTop(row)+Section)continue;
        hit.row=row;
        if(row==DifficultyRow) {
            float left[5],width[5];
            DifficultyEdges(left,width);
            hit.index=0;
            for(int i=1;i<5;i++)if(layerX>=(left[i-1]+width[i-1]+left[i])/2)hit.index=i;
        } else hit.index=layerX<320?0:1;
        return hit;
    }
    return hit;
}

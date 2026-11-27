#include "ReplayPanel.hpp"
#include "ReplayNames.hpp"
#include "ReplayPortraits.hpp"
#include "RecordHeadings.hpp"
#include "MenuCaption.hpp"
#include "ReplayFlow.hpp"
#include "ReplayStageGrid.hpp"
#include "PanelTransition.hpp"
#include "MenuTypeface.hpp"
#include "MenuCanvas.hpp"
#include "Presentation.hpp"
#include "MainMenu.hpp"
#include "ResultScreen.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"
#include "Controller.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace {
constexpr int Rows=11,Body=12,DetailBody=15,Emphasis=DetailBody,StageCount=7,Dimmed=135;
constexpr int ListTop=64,RowStep=15,HeaderY=44,NumberX=58,NameX=88,PlayerX=158,RankX=236;
constexpr int Left=28,Right=292,StageHeaderY=84;
constexpr int StageTop=StageHeaderY+20,Divider=StageHeaderY+88;
constexpr int InfoCenterY=46,InfoRowSpacing=24;
constexpr unsigned White=0xffffff,Muted=0xb8a4a4;
struct Snapshot {
    bool visible=false,saving=false,stageActive=false;
    int selected=0,count=0,cursor=0,state=0,frame=0,error=0;
    std::array<ReplayHeader,15> headers{};
    std::array<std::array<char,8>,15> slots{};
    std::array<bool,StageCount> stagePresent{};
    std::array<int,StageCount> stageScores{};
    ReplayHeader selectedHeader{};
} shown;
CtrPanelTransition<Snapshot> lower;
int entryFrame=0;
float opacity=0;
unsigned Color(unsigned rgb,float alpha) {
    return C2D_Color32(rgb>>16,(rgb>>8)&255,rgb&255,unsigned(255*std::clamp(alpha,0.f,1.f)));
}
std::string Name(const ReplayHeader& header) {
    std::string name(header.name,sizeof(header.name));
    while(!name.empty() && (name.back()==' ' || !name.back()))name.pop_back();
    return name;
}
std::string Date(const ReplayHeader& header) {return std::string(header.date,strnlen(header.date,sizeof(header.date)));}
bool Exists(const ReplayHeader& header) {return !std::memcmp(header.magic,"T6RP",4) && header.version==GAME_VERSION;}
const char* Shot(const ReplayHeader& header) {
    static const char* shots[]={"Reimu A","Reimu B","Marisa A","Marisa B"};
    return shots[std::clamp(int(header.shottypeChara),0,3)];
}
std::string Score(int score) {return std::to_string(std::max(0,score));}
float Width(const std::string& text,int size=Body) {return CtrMenuTypeface::Width(text.c_str(),size);}
void Text(std::string text,float x,int y,int size,unsigned color,int width=320,bool centered=false) {
    if(Width(text,size)>width) {
        do {text.pop_back();}while(!text.empty() && Width(text+"...",size)>width);
        text+="...";
    }
    if(centered)x-=Width(text,size)/2;
    CtrMenuTypeface::Draw(text.c_str(),std::round(x),y,size,color,1000);
}
void Label(const char* name,float x,int y,unsigned color,bool centered=false) {
    CtrRecordHeadings::DrawText(name,std::round(x),y,color,centered);
}
void Arrow(float x,int y,unsigned color) {C2D_DrawTriangle(x,y,color,x+6,y+4,color,x,y+8,color,0);}
void Rule(float left,int y,float right,float alpha) {
    if(right>left)C2D_DrawRectSolid(std::round(left),y,0,std::round(right-left),1,Color(0x783c3c,alpha));
}
bool ListHidden() {return shown.saving && (shown.state==RESULT_SCREEN_STATE_SAVE_REPLAY_QUESTION || shown.state==RESULT_SCREEN_STATE_CANT_SAVE_REPLAY);}
int FirstRow() {return std::clamp(shown.selected-Rows/2,0,std::max(0,shown.count-Rows));}
void ListBox() {
    if(ListHidden())return;
    const int first=FirstRow();
    const int inset=int(CtrMenuCanvas::ScreenLeft());
    CtrPresentation::ShowListBox((52-inset)*2,40*2,((shown.saving?336:292)-inset)*2,230*2,
        first>0,first+Rows<shown.count,opacity);
}
void List(C3D_RenderTarget* top) {
    C2D_SceneBegin(top);
    Label("replay-title",172,8,Color(White,opacity),true);
    Label("replay-subtitle",172,23,Color(White,opacity),true);
    if(ListHidden())return;
    if(!shown.count) {Label("empty",200,132,Color(White,opacity),true);return;}
    const auto header=Color(0xc8b0b0,opacity);
    Label("replay-number",NumberX,HeaderY,header);Label("replay-name-header",NameX,HeaderY,header);
    Label("replay-player",PlayerX,HeaderY,header);Label(shown.saving?"replay-score":"replay-rank",RankX,HeaderY,header);
    Rule(58,58,shown.saving?330:286,opacity);
    const int first=FirstRow();
    for(int row=0;row<Rows && first+row<shown.count;row++) {
        const int index=first+row,y=ListTop+row*RowStep;
        const bool selected=index==shown.selected;
        const auto& header=shown.headers[index];
        const float value=opacity*(selected?1:Dimmed/255.f);
        const auto color=Color(White,value);
        if(selected)Arrow(NumberX-9,y+3,color);
        Text(shown.slots[index].data(),NumberX,y,Body,color,20);
        if(!Exists(header)){Text("--------",NameX,y,Body,color,64);continue;}
        CtrReplayNames::Draw(Name(header).c_str(),NameX,y+7,Body,64,color,false);
        Text(Shot(header),PlayerX,y,Body,color,72);
        if(shown.saving)Text(Score(header.score),RankX,y,Body,color,96);
        else {
            char label[24];std::snprintf(label,sizeof(label),"replay-list-difficulty%d",std::clamp(int(header.difficulty),0,4));
            Label(label,RankX,y,color);
        }
    }
}
void Choices(float alpha) {
    Label("replay-yes",100,132,Color(shown.cursor==0?0xffff80:White,alpha*(shown.cursor==0?1:.6f)),true);
    Label("replay-no",220,132,Color(shown.cursor==1?0xffff80:White,alpha*(shown.cursor==1?1:.6f)),true);
    Arrow(shown.cursor==0?68:188,136,Color(0xffff80,alpha));
}
void SaveScreen(C3D_RenderTarget* bottom) {
    C2D_SceneBegin(bottom);
    const auto white=Color(White,opacity);
    const char* label="choose-slot";
    if(shown.state==RESULT_SCREEN_STATE_SAVE_REPLAY_QUESTION)label="save";
    else if(shown.state==RESULT_SCREEN_STATE_OVERWRITE_REPLAY_FILE)label="overwrite";
    else if(shown.state==RESULT_SCREEN_STATE_WRITING_REPLAY_NAME)label="replay-name";
    if(shown.state==RESULT_SCREEN_STATE_CANT_SAVE_REPLAY) {
        if(shown.error) {
            Label(shown.error==1?"continue-error1":"rate-error1",160,90,white,true);
            Label(shown.error==1?"continue-error2":"rate-error2",160,114,white,true);
        } else Label("save-error",160,98,white,true);
        return;
    }
    Label(label,160,76,white,true);
    if(shown.state==RESULT_SCREEN_STATE_SAVE_REPLAY_QUESTION || shown.state==RESULT_SCREEN_STATE_OVERWRITE_REPLAY_FILE)Choices(opacity);
    else {
        CtrReplayNames::Draw(Name(shown.headers[shown.selected]).c_str(),160,124,Emphasis,264,white);
        Text("No."+std::to_string(shown.selected+1),160,148,Body,Color(Muted,opacity),264,true);
    }
}
struct StageSpacing {int top,slowdown,divider;};
StageSpacing Spacing(const CtrReplayStageGrid& grid) {
    const int height=(std::max(1,grid.rows)-1)*16+DetailBody;
    const int divider=std::max(Divider,((100+height+4+DetailBody+4+3)/4)*4);
    const int available=divider-4-100;
    const int gap=std::clamp(available-height-DetailBody,4,16);
    const int top=100+(available-height-gap-DetailBody)/2;
    return {top,top+height+gap,divider};
}
void StageScores(const Snapshot& detail,float alpha) {
    const CtrReplayStageGrid grid(detail.stagePresent);
    const auto& stages=grid.stages;
    const int count=grid.count,columns=grid.columns,rows=grid.rows;
    const auto spacing=Spacing(grid);
    const int step=16,stageTop=spacing.top;
    for(int column=0;column<columns;column++) {
        const int first=column*rows,last=std::min(count,first+rows);
        float labelWidth=0,scoreWidth=0;
        for(int i=first;i<last;i++) {
            labelWidth=std::max(labelWidth,Width(stages[i]==6?"EX":std::to_string(stages[i]+1),DetailBody));
            scoreWidth=std::max(scoreWidth,Width(Score(detail.stageScores[stages[i]]),DetailBody));
        }
        const int center=columns==1?160:(column==0?94:226);
        const float left=std::round(center-(labelWidth+12+scoreWidth)/2),scoreX=left+labelWidth+12;
        for(int i=first;i<last;i++) {
            const int stage=stages[i],y=stageTop+(i-first)*step;
            const bool selected=detail.stageActive && stage==detail.cursor;
            const auto color=Color(White,alpha*(detail.stageActive && !selected?Dimmed/255.f:1));
            Text(stage==6?"EX":std::to_string(stage+1),left,y,DetailBody,color);
            Text(Score(detail.stageScores[stage]),scoreX,y,DetailBody,color);
            if(selected)Arrow(left-10,y+3,color);
        }
    }
    if(detail.stageActive)return;
    char rate[24];std::snprintf(rate,sizeof(rate),"%.3f%%",detail.selectedHeader.slowdownRate);
    const float total=CtrRecordHeadings::TextWidth("replay-slowdown")+12+Width(rate,DetailBody);
    const float left=std::round(160-total/2);
    const int y=spacing.slowdown;
    CtrRecordHeadings::DrawCaptionAtInk("replay-slowdown",left,y+4,false,alpha);
    Text(rate,left+CtrRecordHeadings::TextWidth("replay-slowdown")+12,y,DetailBody,Color(White,alpha));
}
void Details(const Snapshot& detail,float alpha) {
    if(!detail.visible || !detail.count || alpha<=0)return;
    C2D_ViewReset();
    const auto white=Color(White,alpha);
    const auto& replay=detail.selectedHeader;
    const std::string date=Date(replay);
    const float captionAdvance=CtrRecordHeadings::TextAdvance("replay-caption");
    char slot[24];std::snprintf(slot,sizeof(slot),"replay-slot%s",detail.slots[detail.selected].data());
    CtrRecordHeadings::DrawCaption("replay-caption",Left,20,true,alpha);
    CtrRecordHeadings::DrawCaption(slot,std::round(Left+captionAdvance),20,true,alpha);
    Text(date,Right-Width(date,DetailBody),18,DetailBody,Color(Muted,alpha));
    CtrMenuCaption::Rule(Left,20,captionAdvance+CtrRecordHeadings::CellWidth(slot),Right-Width(date,DetailBody)-8,unsigned(255*alpha));
    CtrReplayNames::Draw(Name(replay).c_str(),160,InfoCenterY,Emphasis,264,Color(0xc8c4ff,alpha));
    char header[24],difficulty[24];
    std::snprintf(header,sizeof(header),"replay-header%d",std::clamp(int(replay.shottypeChara),0,3));
    std::snprintf(difficulty,sizeof(difficulty),"replay-difficulty%d",std::clamp(int(replay.difficulty),0,4));
    const float width=CtrRecordHeadings::TextWidth(header),total=width+12+CtrRecordHeadings::TextWidth(difficulty);
    const float x=std::round(160-total/2);
    const float centerY=InfoCenterY+InfoRowSpacing;
    CtrRecordHeadings::DrawText(header,x,centerY-CtrRecordHeadings::TextHeight(header)/2,white);
    CtrRecordHeadings::DrawText(difficulty,x+width+12,centerY-CtrRecordHeadings::TextHeight(difficulty)/2,white);
    CtrRecordHeadings::DrawCaption("replay-stage",Left,StageHeaderY-2,detail.stageActive,alpha);
    CtrMenuCaption::Rule(Left,StageHeaderY-2,CtrRecordHeadings::CellWidth("replay-stage"),Right,unsigned(255*alpha));
    StageScores(detail,alpha);
    Rule(Left,Spacing(CtrReplayStageGrid(detail.stagePresent)).divider,Right,alpha);
    Label("replay-play",94,196,white,true);
    Label("replay-back",226,196,Color(White,alpha*Dimmed/255),true);
    C2D_ViewReset();
}
void Advance() {
    float fade=CtrMenuTiming::Progress(entryFrame);
    entryFrame=std::min(entryFrame+1,CtrMenuTiming::Frames);
    if(!shown.saving && shown.state==STATE_REPLAY_UNLOAD)fade*=1-CtrMenuTiming::Progress(shown.frame);
    opacity=shown.saving?fade:1;
    if(!shown.saving)CtrPresentation::SetContentOpacity(fade);
    if(shown.saving)lower.Reset();
    else {
        lower.Capture(shown.selected,shown);
    }
    ListBox();
    if(!ListHidden()) {
        const int first=FirstRow();
        for(int row=0;row<Rows && first+row<shown.count;row++)if(Exists(shown.headers[first+row]))
            CtrReplayNames::Prepare(Name(shown.headers[first+row]).c_str(),Body,64);
    }
    if(shown.count)CtrReplayNames::Prepare(Name(shown.headers[shown.selected]).c_str(),Emphasis,264);
}
}
void CtrReplayPanel::Clear(){shown.visible=false;}
void CtrReplayPanel::Release(){CtrReplayNames::Release();CtrReplayPortraits::Release();shown.visible=false;entryFrame=0;lower.Reset();}
bool CtrReplayPanel::Saving(int state) {return state>=RESULT_SCREEN_STATE_SAVE_REPLAY_QUESTION && state<=RESULT_SCREEN_STATE_OVERWRITE_REPLAY_FILE;}
void CtrReplayPanel::Show(const MainMenu& menu) {
    shown={};shown.visible=CtrRecordHeadings::Prepare() && CtrMenuTypeface::Initialize();
    shown.state=menu.gameState;shown.frame=menu.stateTimer;
    shown.count=std::clamp(int(menu.replayFilesNum),0,15);
    shown.selected=std::clamp(int(menu.chosenReplay),0,std::max(0,shown.count-1));shown.cursor=menu.cursor;
    for(int i=0;i<shown.count;i++) {
        shown.headers[i]=*menu.replayFileData[i].header;
        std::snprintf(shown.slots[i].data(),shown.slots[i].size(),"%02d",std::atoi(menu.replayFileName[i]+3));
    }
    if(shown.count) {
        CtrReplayPortraits::Prepare();shown.selectedHeader=shown.headers[shown.selected];
        const auto* header=menu.replayFileData[shown.selected].header;
        for(int i=0;i<StageCount;i++)if(header->stageReplayDataOffsets[i]) {
            const auto* stage=reinterpret_cast<const StageReplayData*>(reinterpret_cast<const unsigned char*>(header)+header->stageReplayDataOffsets[i]);
            shown.stagePresent[i]=true;shown.stageScores[i]=stage->score;
        }
    }
    shown.stageActive=CtrReplayFlow::StageSelectionReady(menu);
    if(shown.visible)Advance();
}
void CtrReplayPanel::ShowSave(const ResultScreen& screen) {
    shown={};shown.saving=true;shown.visible=CtrRecordHeadings::Prepare() && CtrMenuTypeface::Initialize();shown.count=15;
    shown.state=screen.resultScreenState;shown.frame=screen.frameTimer;shown.cursor=screen.cursor;
    shown.selected=std::clamp(int(screen.replayNumber),0,14);
    for(int i=0;i<15;i++){shown.headers[i]=screen.replays[i];std::snprintf(shown.slots[i].data(),shown.slots[i].size(),"%02d",i+1);}
    if(screen.resultScreenState==RESULT_SCREEN_STATE_WRITING_REPLAY_NAME) {
        auto& preview=shown.headers[shown.selected];preview=screen.defaultReplay;
        std::memcpy(preview.magic,"T6RP",4);preview.version=GAME_VERSION;
        preview.shottypeChara=g_GameManager.CharacterShotType();preview.difficulty=g_GameManager.difficulty;
        std::memcpy(preview.name,screen.replayName,8);
    }
    if(g_GameManager.numRetries)shown.error=1;else if(g_Supervisor.framerateMultiplier<.99f)shown.error=2;
    if(shown.visible)Advance();
}
int CtrReplayPanel::TouchButton(int x,int y) {
    if(!shown.visible || shown.saving || !shown.count || y<Spacing(CtrReplayStageGrid(shown.stagePresent)).divider)return 0;
    return x<160?TH_BUTTON_SELECTMENU:TH_BUTTON_RETURNMENU;
}
void CtrReplayPanel::DrawBackground(C3D_RenderTarget* top) {
    if(!shown.visible || shown.saving || !shown.count)return;
    C2D_ViewReset();C2D_SceneBegin(top);
    const int character=shown.selectedHeader.shottypeChara/2;
    const float progress=CtrMenuTiming::Progress(lower.frame*3);
    const bool changing=progress<1 && lower.previous.visible && lower.previous.count && lower.previous.selectedHeader.shottypeChara/2!=character;
    if(changing)CtrReplayPortraits::Draw(lower.previous.selectedHeader.shottypeChara/2,unsigned(160*opacity*(1-progress)));
    CtrReplayPortraits::Draw(character,unsigned(160*opacity*(changing?progress:1)));
}
void CtrReplayPanel::Draw(C3D_RenderTarget* top,C3D_RenderTarget* bottom) {
    if(!shown.visible){entryFrame=0;lower.Reset();return;}
    C2D_ViewReset();List(top);
    C2D_SceneBegin(bottom);
    if(shown.saving)SaveScreen(bottom);
    else {
        Details(shown,opacity);
        C2D_ViewReset();CtrReplayFlow::DrawConfirmation();
    }
    C2D_ViewReset();
}

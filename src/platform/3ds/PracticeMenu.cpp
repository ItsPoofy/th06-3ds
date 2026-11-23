#include "PracticeMenu.hpp"
#include "PracticeSelection.hpp"
#include "MenuTypeface.hpp"
#include "MenuTiming.hpp"
#include "PanelTransition.hpp"
#include "SelectionArt.hpp"
#include "Presentation.hpp"
#include "GameManager.hpp"
#include "MainMenu.hpp"
#include "SoundPlayer.hpp"
#include "practice/Th06.hpp"
#include "Settings.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {
enum class Page { Setup, Options, List };
enum class Field { Stage, Target, Entry, Portion, Frame, Phase, Options, Start,
                   Lives, Bombs, Power, Score, Graze, Points, Rank, RankLock, Dialogue, Repeat, StopAfterSection, FakeShot, Defaults, Choice };
struct Row {std::string label,value;Field field;int argument=0;bool enabled=true;};
struct Panel {std::string heading;std::vector<Row> rows;int cursor=0,first=0;bool centeredLast=false;};
constexpr const char* LockedEntryText="????????";

int SelectableRow(const std::vector<Row>& rows,int start,int direction=1) {
    const int count=rows.size();
    if(!count)return -1;
    int index=(start%count+count)%count;
    for(int visited=0;visited<count;visited++) {
        if(rows[index].enabled)return index;
        index=(index+direction+count)%count;
    }
    return -1;
}

constexpr float RowsTop=38,HeadingY=6,TextHeight=18,ValueRight=300,LabelLeft=26,RowSpacing=23,DividerY=31;
constexpr int VisibleRows=8;
constexpr const char* Routes[]={"Selected","Reimu A","Reimu B","Marisa A","Marisa B"};
constexpr const char* RoutesJa[]={"選択中","霊符","夢符","魔符","恋符"};
constexpr float CardStep=49,CardTop=120-CardStep;
constexpr float ModeScale=.72f;

Page page=Page::Setup;
Field listField=Field::Stage;
int setupCursor=0,optionsCursor=0;
int pageStartTimer=0;
CtrPanelTransition<Panel> transition;
bool backwards=false;
int menuState=STATE_STARTUP,typeTimer=0,typeCursor=0,setupTimer=0;
bool typeBackwards=false;
using Practice::Current;

std::string Number(long long value){return std::to_string(value);}
bool Japanese(){return !CtrSettings::English();}
const char* Tr(const char* english,const char* japanese){return Japanese()?japanese:english;}
const char* RouteName(int route){return (Japanese()?RoutesJa:Routes)[route+1];}
std::string TargetText(Practice::Target target) {
    static constexpr const char* japanese[]={"最初から","道中","中ボス","ボス","通常攻撃","スペルカード","フレーム"};
    return Japanese()?japanese[int(target)]:Practice::TargetName(target);
}
std::string EntryText(const Practice::Entry& entry){return Practice::Th06::Encountered(entry)?Practice::Th06::EntryName(entry,Japanese()):LockedEntryText;}
std::string PhaseText(const Practice::Entry& entry,int phase){return Practice::Th06::PhaseName(entry,phase,Japanese());}
const char* SettingName(int index) {
    static constexpr const char* japanese[]={"残機","ボム","パワー","スコア","グレイズ","点アイテム","ランク"};
    return Japanese()?japanese[index]:Practice::Th06::StartSetting(index,Current().options).name;
}
const char* OnOff(bool on){return on?Tr("On","オン"):Tr("Off","オフ");}
const Practice::Entry* Selected(){return Practice::Th06::Selected();}
int DifficultyIndex(){return std::clamp(int(g_GameManager.difficulty),0,4);}
bool Extra(){return Current().stage==7;}
bool EntryTarget(){const auto t=Current().target;return t>=Practice::Target::Midboss && t<=Practice::Target::Spell;}
std::vector<const Practice::Entry*> Entries(){return Practice::Select(Practice::Th06::Entries(),Current());}
int PortionCount(){const auto& s=Practice::Th06::GetStage(Current().stage);return s.firstPortions+s.secondPortions;}
std::string PortionName(int portion) {
    const auto& stage=Practice::Th06::GetStage(Current().stage);
    if(!stage.secondPortions)return Tr("Portion ","区間")+Number(portion);
    return portion<=stage.firstPortions?Tr("1st Half ","前半")+Number(portion):Tr("2nd Half ","後半")+Number(portion-stage.firstPortions);
}
std::string StageName(int stage){return stage==7?Tr("Extra","エクストラ"):Tr("Stage ","ステージ")+Number(stage);}
std::vector<int> Stages() {
    std::vector<int> result;
    if(g_GameManager.difficulty==EXTRA)return Practice::Th06::StageAvailable(7)?std::vector<int>{7}:std::vector<int>{};
    for(int i=1;i<=6;i++)if(Practice::Th06::StageAvailable(i))result.push_back(i);
    return result;
}
bool TargetAvailable(Practice::Target target) {
    if(target<Practice::Target::Midboss || target>Practice::Target::Spell)return true;
    auto session=Current();session.target=target;
    return !Practice::Select(Practice::Th06::Entries(),session).empty();
}
std::vector<int> Targets() {
    std::vector<int> result;
    for(int i=0;i<7;i++)if(TargetAvailable(Practice::Target(i)))result.push_back(i);
    return result;
}
bool Startable() {
    if(!Practice::Th06::StageAvailable(Current().stage))return false;
    if(!EntryTarget())return true;
    const auto* entry=Selected();return entry && Practice::Th06::Encountered(*entry);
}
void Changed(){Practice::Th06::Normalize();}

std::vector<Row> SetupRows() {
    auto& s=Current();std::vector<Row> rows;
    const bool available=Practice::Th06::StageAvailable(s.stage);
    rows.push_back({Tr("Stage","ステージ"),available?StageName(s.stage):LockedEntryText,Field::Stage,0,!Extra() && Stages().size()>1});
    rows.push_back({Tr("Start","開始地点"),TargetText(s.target),Field::Target,0,available});
    switch(s.target) {
    case Practice::Target::Portion:rows.push_back({Tr("Portion","区間"),PortionName(s.portion),Field::Portion});break;
    case Practice::Target::Frame:rows.push_back({Tr("Frame","フレーム"),Number(s.frame),Field::Frame});break;
    case Practice::Target::Beginning:break;
    default:{
        const auto* entry=Selected();const auto entries=Entries();
        const bool unlocked=std::any_of(entries.begin(),entries.end(),[](const auto* item){return Practice::Th06::Encountered(*item);});
        rows.push_back({Tr("Section","場面"),entry?EntryText(*entry):LockedEntryText,Field::Entry,0,available && unlocked});
    }
    }
    if(const auto* entry=Selected();EntryTarget() && entry && Practice::Th06::Encountered(*entry) && !entry->phases.empty())
        rows.push_back({Tr("Phase","フェーズ"),PhaseText(*entry,s.phase),Field::Phase});
    rows.push_back({Tr("Options","オプション"),"",Field::Options});
    rows.push_back({Tr("Start Practice","練習開始"),"",Field::Start,0,Startable()});
    return rows;
}
std::vector<Row> OptionRows() {
    const auto& o=Current().options;std::vector<Row> rows;
    const Field fields[]={Field::Lives,Field::Bombs,Field::Power,Field::Score,Field::Graze,Field::Points,Field::Rank};
    for(int i=0;i<7;i++)rows.push_back({SettingName(i),Number(Practice::Th06::Value(i,o)),fields[i],i});
    rows.push_back({Tr("Rank Lock","ランク固定"),OnOff(o.rankLock),Field::RankLock});
    rows.push_back({Tr("Repeat Until Stopped","停止するまで繰り返す"),OnOff(o.repeat),Field::Repeat});
    rows.push_back({Tr("Stop After Section","区間終了で停止"),OnOff(o.stopAfterSection),Field::StopAfterSection});
    const auto* entry=Selected();
    rows.push_back({Tr("Skip Dialogue","会話スキップ"),OnOff(o.skipDialogue),Field::Dialogue,0,EntryTarget() && entry && entry->dialogue});
    if(Current().stage==4 && Current().target!=Practice::Target::Beginning)rows.push_back({Tr("Fake Shot","機体偽装"),RouteName(o.shotRoute),Field::FakeShot});
    rows.push_back({Tr("Reset Defaults","初期設定に戻す"),"",Field::Defaults});
    return rows;
}
std::vector<Row> ListRows(int& current) {
    auto& s=Current();std::vector<Row> rows;current=0;
    switch(listField) {
    case Field::Stage:for(int stage:Stages()){if(stage==s.stage)current=rows.size();rows.push_back({StageName(stage)+"  "+Practice::Th06::StageName(stage,Japanese()),"",Field::Choice,stage});}break;
    case Field::Target:for(int t:Targets()){if(t==int(s.target))current=rows.size();rows.push_back({TargetText(Practice::Target(t)),"",Field::Choice,t});}break;
    case Field::Portion:for(int i=1;i<=PortionCount();i++){if(i==s.portion)current=rows.size();rows.push_back({PortionName(i),"",Field::Choice,i});}break;
    case Field::Entry: {
        const auto entries=Entries();
        for(int i=0;i<int(entries.size());i++) {
            if(entries[i]->id==s.entry)current=i;
            rows.push_back({EntryText(*entries[i]),"",Field::Choice,i,Practice::Th06::Encountered(*entries[i])});
        }
        break;
    }
    case Field::Phase:if(const auto* entry=Selected())for(int i=0;i<int(entry->phases.size());i++){rows.push_back({PhaseText(*entry,i),"",Field::Choice,i});}current=s.phase;break;
    case Field::FakeShot:for(int i=0;i<5;i++)rows.push_back({RouteName(i-1),"",Field::Choice,i-1});current=s.options.shotRoute+1;break;
    default:break;
    }
    return rows;
}
std::vector<Row> Rows(){int current;return page==Page::Setup?SetupRows():page==Page::Options?OptionRows():ListRows(current);}
std::string Heading() {
    if(page==Page::Setup)return Tr("Custom Practice","カスタム練習");
    if(page==Page::Options)return Tr("Options","オプション");
    switch(listField) {
    case Field::Stage:return Tr("Stage","ステージ");case Field::Target:return Tr("Start","開始地点");case Field::Portion:return Tr("Portion","区間");
    case Field::Phase:return Tr("Phase","フェーズ");case Field::FakeShot:return Tr("Fake Shot","機体偽装");
    default:return TargetText(Current().target);
    }
}
void Sound(SoundIdx sound){g_SoundPlayer.PlaySoundByIdx(sound);}
template<class T> int Step(const std::vector<T>& values,const T& current,int direction) {
    const int count=values.size();if(!count)return -1;
    int index=0;for(int i=0;i<count;i++)if(values[i]==current)index=i;
    return (index+direction+count)%count;
}
bool Cycle(Field field,int direction) {
    auto& s=Current();
    switch(field) {
    case Field::Stage: {
        const auto stages=Stages();if(stages.size()<2)return false;
        s.stage=stages[Step(stages,s.stage,direction)];s.entry.clear();s.phase=0;
        if(!TargetAvailable(s.target))s.target=Practice::Target::Beginning;
        break;
    }
    case Field::Target: {
        const auto targets=Targets();s.target=Practice::Target(targets[Step(targets,int(s.target),direction)]);
        s.entry.clear();s.phase=0;break;
    }
    case Field::Portion:s.portion=(s.portion-1+direction+PortionCount())%PortionCount()+1;break;
    case Field::Frame:s.frame=std::max(0,s.frame+direction*60);break;
    case Field::Entry: {
        const auto entries=Entries();std::vector<const Practice::Entry*> open;
        for(const auto* entry:entries)if(Practice::Th06::Encountered(*entry))open.push_back(entry);
        if(open.size()<2)return false;
        const auto* current=Selected();s.entry=open[Step(open,current,direction)]->id;s.phase=0;break;
    }
    case Field::Phase: {
        const auto* entry=Selected();if(!entry || entry->phases.size()<2)return false;
        s.phase=(s.phase+direction+int(entry->phases.size()))%int(entry->phases.size());break;
    }
    default:return false;
    }
    Changed();return true;
}
bool Adjust(Field field,int argument,int direction) {
    auto& o=Current().options;
    switch(field) {
    case Field::Lives:case Field::Bombs:case Field::Power:case Field::Score:case Field::Graze:case Field::Points:case Field::Rank: {
        // Larger values move in larger steps so the D-pad can reach them.
        static constexpr long long steps[]={1,1,1,1000000,100,10,1};
        const auto setting=Practice::Th06::StartSetting(argument,o);
        long long value=Practice::Th06::Value(argument,o)+direction*steps[argument];
        if(value>setting.maximum)value=setting.minimum;else if(value<setting.minimum)value=setting.maximum;
        Practice::Th06::SetValue(argument,value,o);break;
    }
    case Field::RankLock:o.rankLock=!o.rankLock;Practice::Th06::SetValue(6,o.rank,o);break;
    case Field::Dialogue:o.skipDialogue=!o.skipDialogue;break;
    case Field::Repeat:o.repeat=!o.repeat;break;
    case Field::StopAfterSection:o.stopAfterSection=!o.stopAfterSection;break;
    case Field::FakeShot:o.shotRoute=(o.shotRoute+1+direction+5)%5-1;Changed();break;
    default:return false;
    }
    return true;
}
void Show(Page next,MainMenu& menu,int cursor,bool back) {
    if(page==Page::Setup)setupCursor=menu.cursor;
    if(page==Page::Options)optionsCursor=menu.cursor;
    page=next;menu.cursor=cursor;backwards=back;
    menu.cursor=SelectableRow(Rows(),menu.cursor);
    pageStartTimer=menu.stateTimer;
}
void Back(MainMenu& menu) {
    if(page==Page::List && listField==Field::FakeShot)Show(Page::Options,menu,optionsCursor,true);
    else Show(Page::Setup,menu,setupCursor,true);
}
void Choose(const Row& row,MainMenu& menu) {
    auto& s=Current();
    switch(listField) {
    case Field::Stage:s.stage=row.argument;s.entry.clear();s.phase=0;if(!TargetAvailable(s.target))s.target=Practice::Target::Beginning;break;
    case Field::Target:s.target=Practice::Target(row.argument);s.entry.clear();s.phase=0;break;
    case Field::Portion:s.portion=row.argument;break;
    case Field::Entry:s.entry=Entries()[row.argument]->id;s.phase=0;break;
    case Field::Phase:s.phase=row.argument;break;
    case Field::FakeShot:s.options.shotRoute=row.argument;break;
    default:break;
    }
    Changed();Back(menu);
}
void Move(MainMenu& menu,const std::vector<Row>& rows) {
    const int previous=menu.cursor;
    const int direction=WAS_PRESSED_PERIODIC(TH_BUTTON_UP)?-1:WAS_PRESSED_PERIODIC(TH_BUTTON_DOWN)?1:0;
    const int start=direction?menu.cursor+direction:menu.cursor;
    menu.cursor=SelectableRow(rows,start,direction?direction:1);
    if(previous!=menu.cursor)Sound(SOUND_MOVE_MENU);
}
int First(int cursor,int count){return std::clamp(cursor-VisibleRows/2,0,std::max(0,count-VisibleRows));}

void Text(const std::string& value,float x,float y,float height,unsigned color,float width=300,bool centered=false) {
    CtrMenuTypeface::Draw(value.c_str(),x,y,height,color,width,centered);
}
unsigned RowColor(bool selected,bool enabled,unsigned alpha) {
    if(!enabled)return C2D_Color32(125,125,125,alpha*128/255);
    return selected?C2D_Color32(255,255,128,alpha):C2D_Color32(255,255,255,alpha*128/255);
}
void DrawPanel(const Panel& p,float slide,float opacity) {
    const auto alpha=unsigned(opacity*CtrPresentation::SceneOpacity());
    if(!alpha)return;
    Text(p.heading,160+slide,HeadingY,TextHeight,C2D_Color32(255,255,255,alpha),300,true);
    const unsigned divider=C2D_Color32(94,51,45,alpha);
    C2D_DrawLine(28+slide,DividerY,divider,292+slide,DividerY,divider,1,0);
    const int count=p.rows.size();
    const float spacing=RowSpacing;
    for(int i=p.first;i<std::min(count,p.first+VisibleRows+1);i++) {
        if(i-p.first>=VisibleRows && !(p.centeredLast && i==count-1))continue;
        const auto& row=p.rows[i];const bool selected=i==p.cursor;
        const float y=RowsTop+(i-p.first)*spacing-(selected?1:0);
        const unsigned color=RowColor(selected,row.enabled,alpha);
        if(p.centeredLast && i==count-1) {
            const float top=RowsTop+std::min(count-1,VisibleRows)*spacing+6;
            C2D_DrawLine(28+slide,top-4,divider,292+slide,top-4,divider,1,0);
            Text(row.label,160+slide,top-(selected?1:0),TextHeight,color,280,true);
            continue;
        }
        const float valueWidth=row.value.empty()?0:std::min(CtrMenuTypeface::Width(row.value.c_str(),TextHeight),row.value.size()>12?196.f:150.f);
        Text(row.label,LabelLeft+slide,y,TextHeight,color,row.value.empty()?280:ValueRight-LabelLeft-valueWidth-12);
        if(!row.value.empty())Text(row.value,ValueRight-valueWidth+slide,y,TextHeight,color,valueWidth);
    }
}
void DrawModeCards(float slide,unsigned alpha,int selected) {
    for(int i=0;i<2;i++) {
        const bool highlight=i==selected;const float y=CardTop+CardStep*i;
        CtrSelectionArt::Draw("difficulty-card",20+slide,y,C2D_Color32(highlight?255:140,highlight?255:140,highlight?255:140,alpha));
        CtrMenuTypeface::DrawMode(i,160+slide,y+23,(highlight?255:190)*alpha/255,ModeScale);
    }
}
}

void CtrPracticeMenu::ResetPresentation() {
    transition={};page=Page::Setup;listField=Field::Stage;
    setupCursor=optionsCursor=typeCursor=typeTimer=setupTimer=0;
    pageStartTimer=0;
    menuState=STATE_STARTUP;backwards=typeBackwards=false;
}
void CtrPracticeMenu::Begin() {Practice::Reset();ResetPresentation();}
void CtrPracticeMenu::OpenSetup(MainMenu& menu,bool atStart) {
    auto& s=Current();
    s.shot=g_GameManager.CharacterShotType();s.difficulty=g_GameManager.difficulty;
    if(g_GameManager.difficulty==EXTRA)s.stage=7;
    else if(s.stage==7 || !Practice::Th06::StageAvailable(s.stage)) {
        const auto stages=Stages();s.stage=stages.empty()?1:stages.front();
    }
    if(!TargetAvailable(s.target))s.target=Practice::Target::Beginning;
    Changed();
    menu.gameState=STATE_PRACTICE_SETUP;menu.stateTimer=0;
    page=Page::Setup;setupCursor=optionsCursor=0;backwards=false;
    pageStartTimer=0;
    const auto rows=SetupRows();
    menu.cursor=SelectableRow(rows,atStart?int(rows.size())-1:0);
    transition.Reset();transition.key=1000;transition.current={};transition.previous={};
}
int CtrPracticeMenu::ChooseMode(MainMenu& menu) {
    if(menu.stateTimer<CtrMenuTiming::Frames)return 0;
    const int previous=menu.cursor;
    if(WAS_PRESSED_PERIODIC(TH_BUTTON_UP|TH_BUTTON_DOWN))menu.cursor=1-menu.cursor;
    if(previous!=menu.cursor)Sound(SOUND_MOVE_MENU);
    if(WAS_PRESSED(TH_BUTTON_RETURNMENU)){Sound(SOUND_BACK);return -1;}
    if(WAS_PRESSED(TH_BUTTON_SELECTMENU)) {
        Current().mode=menu.cursor?Practice::Mode::Custom:Practice::Mode::Regular;Sound(SOUND_SELECT);return 1;
    }
    return 0;
}
int CtrPracticeMenu::Update(MainMenu& menu) {
    if(menu.stateTimer<CtrMenuTiming::Frames || transition.frame<CtrMenuTiming::Frames)return 0;
    const auto rows=Rows();Move(menu,rows);
    if(WAS_PRESSED(TH_BUTTON_RETURNMENU)) {
        if(page==Page::Setup){setupCursor=menu.cursor;return -1;}
        Back(menu);Sound(SOUND_BACK);return 0;
    }
    if(menu.cursor<0)return 0;
    const auto row=rows[menu.cursor];
    if(!row.enabled)return 0;
    const int direction=WAS_PRESSED_PERIODIC(TH_BUTTON_LEFT)?-1:WAS_PRESSED_PERIODIC(TH_BUTTON_RIGHT)?1:0;
    if(direction && page!=Page::List) {
        if(page==Page::Setup?Cycle(row.field,direction):Adjust(row.field,row.argument,direction))Sound(SOUND_MOVE_MENU);
        return 0;
    }
    if(!WAS_PRESSED(TH_BUTTON_SELECTMENU))return 0;
    switch(row.field) {
    case Field::Stage:case Field::Target:case Field::Portion:case Field::Entry:case Field::Phase:case Field::FakeShot: {
        listField=row.field;int current=0;
        const auto choices=ListRows(current);
        if(SelectableRow(choices,current)<0){Sound(SOUND_BACK);break;}
        Show(Page::List,menu,current,false);Sound(SOUND_SELECT);break;
    }
    case Field::Options:Show(Page::Options,menu,0,false);Sound(SOUND_SELECT);break;
    case Field::Defaults:Current().options=Practice::Options{};Changed();Sound(SOUND_SELECT);break;
    case Field::Choice:Choose(row,menu);Sound(SOUND_SELECT);break;
    case Field::Start:break;
    default:if(Adjust(row.field,row.argument,1))Sound(SOUND_MOVE_MENU);break;
    }
    return 0;
}
bool CtrPracticeMenu::CanStart(const MainMenu& menu) {
    if(menu.gameState!=STATE_PRACTICE_SETUP || page!=Page::Setup || transition.frame<CtrMenuTiming::Frames)return false;
    const auto rows=SetupRows();
    return menu.cursor>=0 && menu.cursor<int(rows.size()) && rows[menu.cursor].field==Field::Start && rows[menu.cursor].enabled;
}
bool CtrPracticeMenu::Touch(MainMenu& menu,int x,int y) {
    auto press=[](unsigned button){g_CurFrameInput|=button;g_LastFrameInput&=~button;};
    if(menu.gameState==STATE_PRACTICE_TYPE) {
        if(menu.stateTimer<CtrMenuTiming::Frames)return true;
        for(int i=0;i<2;i++)if(x>=20 && x<300 && y>=CardTop+CardStep*i && y<CardTop+CardStep*i+48) {
            if(menu.cursor!=i){menu.cursor=i;Sound(SOUND_MOVE_MENU);}
            else press(TH_BUTTON_SELECTMENU);
        }
        return true;
    }
    if(menu.gameState!=STATE_PRACTICE_SETUP)return false;
    if(menu.stateTimer<CtrMenuTiming::Frames || transition.frame<CtrMenuTiming::Frames)return true;
    const auto rows=Rows();const int count=rows.size();
    const float spacing=RowSpacing;const int first=First(menu.cursor,count);
    const bool centeredLast=page==Page::Setup;
    for(int i=first;i<count;i++) {
        const bool last=centeredLast && i==count-1;
        const float top=last?RowsTop+std::min(count-1,VisibleRows)*spacing+6:RowsTop+(i-first)*spacing;
        if(!last && i-first>=VisibleRows)break;
        if(y<top-4 || y>=top+spacing-4)continue;
        if(!rows[i].enabled)return true;
        if(menu.cursor!=i){menu.cursor=i;Sound(SOUND_MOVE_MENU);return true;}
        const bool value=page!=Page::List && !rows[i].value.empty() && x>=160;
        press(value?TH_BUTTON_RIGHT:TH_BUTTON_SELECTMENU);
        return true;
    }
    return true;
}
void CtrPracticeMenu::Capture(const MainMenu& menu) {
    if(menu.gameState==STATE_PRACTICE_TYPE) {
        if(menuState!=STATE_PRACTICE_TYPE)typeBackwards=menuState==STATE_DIFFICULTY_SELECT;
        typeTimer=menu.stateTimer;
        typeCursor=menu.cursor;
    }
    if(menu.gameState==STATE_PRACTICE_SETUP)setupTimer=menu.stateTimer;
    menuState=menu.gameState;
    if(menuState!=STATE_PRACTICE_SETUP)return;
    const auto rows=Rows();
    const int cursor=menu.cursor>=0 && menu.cursor<int(rows.size()) && rows[menu.cursor].enabled?int(menu.cursor):-1;
    Panel panel{Heading(),rows,cursor,First(menu.cursor,rows.size()),page==Page::Setup};
    transition.Capture(int(page)*100+int(listField),panel);
    transition.frame=std::clamp<i32>(menu.stateTimer-pageStartTimer,0,CtrMenuTiming::Frames);
}
float CtrPracticeMenu::TypeProgress() {return CtrMenuTiming::SlideProgress(typeTimer);}
void CtrPracticeMenu::DrawTypeCards(float slide,unsigned alpha) {
    CtrPractice::DrawPrompt(STATE_PRACTICE_TYPE,alpha,slide);DrawModeCards(slide,alpha,typeCursor);
}
namespace {
void DrawSummary(float slide,unsigned a) {
    if(!a)return;
    const float x=272+slide;
    const auto& s=Current();
    char stage[16];std::snprintf(stage,sizeof(stage),"Stage %d",s.stage);
    CtrSelectionArt::Draw(stage,x,64,CtrSelectionArt::DifficultyColor(DifficultyIndex(),a),true);
    Text(TargetText(s.target),x,100,TextHeight,C2D_Color32(255,255,255,a),220,true);
    std::string detail;
    if(s.target==Practice::Target::Portion)detail=PortionName(s.portion);
    else if(s.target==Practice::Target::Frame)detail=Tr("Frame ","フレーム")+Number(s.frame);
    else if(EntryTarget()) {
        const auto* entry=Selected();
        if(page==Page::List && listField==Field::Entry) {
            const auto entries=Entries();const int cursor=transition.current.cursor;
            if(cursor>=0 && cursor<int(entries.size()))entry=entries[cursor];
        }
        detail=entry?EntryText(*entry):LockedEntryText;
        if(entry && Practice::Th06::Encountered(*entry) && entry->spell>=0) {
            const auto& record=g_GameManager.catk[entry->spell];
            Text(Tr("History ","履歴 ")+Number(record.numSuccess)+"/"+Number(record.numAttempts),x,152,TextHeight,C2D_Color32(255,255,255,a),220,true);
        }
    }
    if(!detail.empty())Text(detail,x,126,TextHeight,C2D_Color32(255,255,128,a),228,true);
}
}
void CtrPracticeMenu::DrawTop() {
    const unsigned alpha=CtrPresentation::SceneOpacity();
    if(menuState==STATE_PRACTICE_TYPE) {
        const float p=TypeProgress(),direction=typeBackwards?-1.f:1.f;
        DrawTypeTop(direction*400*(1-p),unsigned(alpha*p));
        return;
    }
    if(menuState!=STATE_PRACTICE_SETUP)return;
    const float p=CtrMenuTiming::SlideProgress(setupTimer);
    DrawSummary(400*(1-p),unsigned(alpha*p));
}
void CtrPracticeMenu::DrawTypeTop(float slide,unsigned alpha) {
    CtrMenuTypeface::DrawMode(typeCursor,200+slide,25,alpha,1);
    CtrPractice::DrawDescription(typeCursor,alpha,slide);
}
void CtrPracticeMenu::DrawTopExit(float progress) {
    DrawSummary(400*progress,unsigned(CtrPresentation::SceneOpacity()*(1-progress)));
}
void CtrPracticeMenu::DrawBottom() {
    const unsigned alpha=CtrPresentation::SceneOpacity();
    if(menuState==STATE_PRACTICE_TYPE) {
        const float p=TypeProgress(),direction=typeBackwards?-1.f:1.f;
        CtrPractice::DrawPrompt(STATE_PRACTICE_TYPE,unsigned(alpha*p),direction*320*(1-p));
        DrawModeCards(direction*320*(1-p),alpha,typeCursor);
        return;
    }
    if(menuState!=STATE_PRACTICE_SETUP)return;
    const float progress=CtrMenuTiming::SlideProgress(transition.frame),direction=backwards?-1.f:1.f;
    if(progress<1)DrawPanel(transition.previous,-direction*320*progress,1-progress);
    DrawPanel(transition.current,direction*320*(1-progress),progress);
}
void CtrPracticeMenu::DrawExit(float progress) {
    DrawPanel(transition.current,320*progress,1-progress);
}

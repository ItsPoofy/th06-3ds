#include "Th06Run.hpp"
#include "Completion.hpp"
#include "RunTransition.hpp"
#include "Th06.hpp"
#include "Th06NativeStarts.hpp"
#include "EclEntry.hpp"
#include "AnmManager.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "GameErrorContext.hpp"
#include "Gui.hpp"
#include "platform/3ds/GameDecorations.hpp"
#include "platform/3ds/BossPresentation.hpp"
#include "platform/3ds/PracticeReturn.hpp"
#include "platform/3ds/PauseMenu.hpp"
#include "platform/3ds/DialogueReveal.hpp"
#include "BulletManager.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace {
using namespace Practice;
using namespace Practice::Th06Run;
EclEntry graph;
ActorStart actorStart{};
std::vector<Link> path;
std::vector<const EclRawInstr*> visited;
Enemy* actor=nullptr;
const Entry* selected=nullptr;
bool preparing=false,waiting=false,dialogueSeen=false,healthPending=false;
int targetSub=-1,seekTime=-1,routeOverride=-1;
bool qedRage=false;
bool sectionReady=false;
bool outroBlocked=false;
bool failed=false;
int sectionEndFrame=-1;

void SeekTimeline(int frame) {
    auto* instruction=g_EclManager.timeline;
    while(instruction->time>=0 && instruction->time<frame)
        instruction=reinterpret_cast<const EclTimelineInstr*>(reinterpret_cast<const unsigned char*>(instruction)+instruction->size);
    g_EnemyManager.timelineInstr=instruction;g_EnemyManager.timelineTime.SetCurrent(frame);
}
void Settle(Enemy& enemy) {
    if(enemy.flags.unk1==3)enemy.position=enemy.moveInterpStartPos+enemy.moveInterp;
    enemy.axisSpeed=ZunVec3(0,0,0);enemy.speed=0;enemy.flags.unk1=0;
    enemy.ClampPos();
}
bool PrepareSub(int sub) {
    g_EclManager.CallEclSub(&actor->currentContext,sub);
    visited.clear();preparing=true;
    const bool result=g_EclManager.RunEcl(actor)==ZUN_SUCCESS;
    preparing=false;Settle(*actor);return result;
}
// Extra's midboss is Patchouli. Load Remilia/Flandre assets only for end bosses.
void BossAssets() {
    if(!selected->midboss && Current().stage>=6)g_Gui.PrepareBossAnimations(0);
}
void PlayBossMusic() {
    const int song=selected->midboss?0:1;
    g_Supervisor.PlayAudio(g_Stage.stdData->songPaths[song]);
    g_Gui.ShowSongTitle(song);
}
bool EnterSelected() {
    BossAssets();
    qedRage=selected->spell==63 && Current().phase==2;
    int current=actorStart.sub;
    if(path.empty())g_EclManager.CallEclSub(&actor->currentContext,targetSub);
    for(const auto& link:path) {
        if(!PrepareSub(current))return false;
        const int opcode=link.instruction->opCode;
        if(opcode==ECL_OPCODE_LIFECALLBACKSUB && actor->lifeCallbackSub==link.to && actor->lifeCallbackThreshold>=0) {
            actor->life=actor->lifeCallbackThreshold-1;actor->HandleLifeCallback();
        } else if(opcode==ECL_OPCODE_TIMERCALLBACKSUB && actor->timerCallbackSub==link.to && actor->timerCallbackThreshold>=0) {
            actor->bossTimer.SetCurrent(actor->timerCallbackThreshold);actor->HandleTimerCallback();
        } else {
            g_EclManager.CallEclSub(&actor->currentContext,link.to);actor->stackDepth=0;
            if(opcode==ECL_OPCODE_CALL || (opcode>=ECL_OPCODE_CALLLSS && opcode<=ECL_OPCODE_CALLNEQ)) {
                actor->currentContext.var0=link.instruction->args.call.var0;
                actor->currentContext.float0=link.instruction->args.call.float0;
            }
        }
        current=link.to;
    }
    actor->bossTimer.InitializeForPopup();
    actor->runInterrupt=-1;actor->shootInterval=0;
    g_GameManager.isTimeStopped=false;
    if(g_EclManager.RunEcl(actor)!=ZUN_SUCCESS)return false;
    // Advance the native clock for later survival-card phases.
    if(selected->spell==62 && Current().phase>0) {
        seekTime=Current().phase==1?1976:4536;
        visited.clear();preparing=true;
        if(g_EclManager.RunEcl(actor)!=ZUN_SUCCESS){preparing=false;return false;}
        preparing=false;
        actor->currentContext.time.SetCurrent(seekTime-45);
        actor->bossTimer.SetCurrent(seekTime-45-120);seekTime=-1;
    }
    healthPending=selected->spell==63 && Current().phase==1;
    g_EnemyManager.timelineInstr=actorStart.resume;
    g_EnemyManager.timelineTime.SetCurrent(std::max(int(actorStart.resume->time),actorStart.time));
    // A dialogue that ran has already started its own music and title.
    if(!g_GameManager.demoMode && !dialogueSeen)PlayBossMusic();
    sectionReady=true;
    return true;
}
}
bool Practice::Th06Run::Begin() {
    End();
    auto& session=Current();
    if(!session.Custom() || !g_GameManager.isInPracticeMode || g_GameManager.isInReplay) {
        RunTransition::Reset();
        return true;
    }
    if(!Th06::StageAvailable(session.stage))return false;
    if(!CtrPracticeReturn::Initialize())return false;
    session.running=true;routeOverride=session.options.shotRoute;
    sectionEndFrame=Th06::SectionEndFrame(session);
    const auto& options=session.options;
    const int score=std::min<std::int64_t>(options.score,999999990);
    g_GameManager.score=g_GameManager.guiScore=score;
    g_GameManager.livesRemaining=options.lives;g_GameManager.bombsRemaining=options.bombs;
    g_GameManager.currentPower=options.power;
    g_GameManager.grazeInStage=g_GameManager.grazeInTotal=options.graze;
    g_GameManager.pointItemsCollectedInStage=g_GameManager.pointItemsCollected=options.points;
    g_GameManager.rank=options.rank;
    if(options.rankLock)g_GameManager.minRank=g_GameManager.maxRank=options.rank;
    if(session.difficulty!=EXTRA)g_GameManager.extraLives=score>=60000000?4:score>=40000000?3:score>=20000000?2:score>=10000000?1:0;
    if(session.target==Target::Beginning)return true;
    if(session.target==Target::Portion){SeekTimeline(Th06::PortionFrame(session.stage,session.portion));return true;}
    if(session.target==Target::Frame){SeekTimeline(session.frame);return true;}
    selected=Th06::Selected();
    if(!selected || !Th06::Encountered(*selected))return false;
    g_Gui.HideStageTitle();
    CtrDecorations::PracticeBoss(selected->midboss);
    for(const auto& start:Nonspells)if(selected->id==start.id)targetSub=start.sub;
    if(targetSub==-2){SeekTimeline(3392);return true;}
    if(!graph.Load(g_EclManager,g_EclManager.fileSize,session.difficulty))return false;
    if(selected->spell>=0)targetSub=graph.SpellSub(selected->spell);
    if(!graph.Actor(selected->midboss,actorStart) || !graph.Path(actorStart.sub,targetSub,path))return false;
    if(selected->dialogue && !options.skipDialogue) {
        SeekTimeline(actorStart.dialogueTime);waiting=true;dialogueSeen=false;return true;
    }
    const auto* event=actorStart.instruction;
    ZunVec3 position;position=*event->args.Var1AsVec();
    actor=g_EnemyManager.SpawnEnemy(actorStart.sub,&position,event->args.ushortVar1,event->args.ushortVar2,event->args.uintVar4);
    return EnterSelected();
}
void Practice::Th06Run::End() {
    CtrPracticeReturn::Shutdown();
    Completion::Reset();
    if(!RunTransition::ExitPresented())RunTransition::Reset();
    outroBlocked=failed=false;sectionEndFrame=-1;
    Current().running=false;routeOverride=-1;qedRage=sectionReady=false;
    preparing=waiting=dialogueSeen=healthPending=false;actor=nullptr;selected=nullptr;targetSub=seekTime=-1;
    path.clear();visited.clear();graph.Clear();actorStart={};
}
void Practice::Th06Run::Tick() {
    if(!Current().Custom() || !Current().running)return;
    if(Completion::Active()) {
        const bool nameFinished=!g_Gui.impl->enemySpellcardName.flags.isVisible;
        const bool scriptFinished=outroBlocked || !actor || !actor->flags.active || !g_Gui.BossPresent();
        CtrPracticeReturn::Update();
        const auto action=Completion::Tick(failed || (scriptFinished && nameFinished && !CtrBossPresentation::DefeatPlaying()));
        if(action!=Completion::Action::None) {
            Current().running=false;
            Current().reopen=action==Completion::Action::ReturnPractice;
            if(action==Completion::Action::Retry)CtrPause::RestartRun();
            else g_Supervisor.curState=SUPERVISOR_STATE_MAINMENU;
        }
        return;
    }
    if(waiting) {
        if(g_Gui.HasCurrentMsgIdx())dialogueSeen=true;
        if(dialogueSeen && !g_Gui.HasCurrentMsgIdx() && actor) {
            waiting=false;
            if(!EnterSelected()) {
                g_GameErrorContext.Log("Unable to initialize the selected practice section\n");
                g_Supervisor.curState=SUPERVISOR_STATE_MAINMENU;
            }
        }
    }
    if(healthPending && actor && actor->bossTimer.current>=3){actor->life=1800;healthPending=false;}
}
void Practice::Th06Run::Spawned(Enemy* enemy,int sub) {
    if(!waiting || actor || sub!=actorStart.sub)return;
    // Track the entrance actor before BOSSSET so early dialogue exit can resume it.
    actor=enemy;g_EnemyManager.bosses[0]=enemy;
}
bool Practice::Th06Run::Preparing(){return preparing;}
bool Practice::Th06Run::AttackEnded(Enemy* enemy,bool spell) {
    auto& session=Current();
    if(!session.running || !sectionReady || preparing || waiting || enemy!=actor || !selected ||
       (selected->spell>=0)!=spell || !session.options.stopAfterSection)return false;
    if(Completion::Active())return true;
    Completion::Begin(session.options.repeat);
    enemy->lifeCallbackThreshold=enemy->timerCallbackThreshold=-1;
    g_BulletManager.DespawnBullets(12800,false);
    return true;
}
void Practice::Th06Run::StageEnded() {
    if(!Current().Custom() || !Current().running || Completion::Active())return;
    if(g_Gui.HasCurrentMsgIdx()) {
        g_Gui.impl->msg.currentMsgIdx=-1;
        CtrDialogue::Reset();CtrDecorations::EndDialogue();
    }
    g_GameManager.isTimeStopped=false;
    g_BulletManager.DespawnBullets(12800,false);
    if(g_EnemyManager.spellcardInfo.isActive) {
        g_Gui.EndEnemySpellcard();g_EnemyManager.spellcardInfo.isActive=0;
        g_Stage.spellcardState=NOT_RUNNING;
    }
    Completion::Begin(Current().options.repeat);
}
bool Practice::Th06Run::PlayerDied() {
    auto& session=Current();
    if(!session.Custom() || !session.running || !g_GameManager.isInPracticeMode ||
       g_GameManager.isInReplay || g_GameManager.demoMode)return false;
    if(g_GameManager.livesRemaining>0 && !Completion::Active())return false;
    if(Completion::Leaving())return true;
    if(session.RetriesDeath()) {
        Leave(Completion::Action::Retry);
    } else if(!Completion::Active()) {
        failed=outroBlocked=true;
        StageEnded();
    }
    return true;
}
bool Practice::Th06Run::Leave(Completion::Action action) {
    if(!Current().Custom() || !Current().running || !g_GameManager.isInPracticeMode ||
       g_GameManager.isInReplay || g_GameManager.demoMode)return false;
    if(Completion::Leaving())return true;
    failed=outroBlocked=true;
    StageEnded();Completion::BeginExit(action);
    return true;
}
void Practice::Th06Run::BeforeTimeline() {
    if(Current().running && !Completion::Active() && sectionEndFrame>=0 &&
       g_EnemyManager.timelineTime.current>=sectionEndFrame)StageEnded();
}
bool Practice::Th06Run::ContinueOutro(Enemy* enemy,const EclRawInstr* instruction) {
    if(!Completion::Active())return true;
    if(enemy!=actor || outroBlocked || Completion::CountingDown())return false;
    // Finish defeat effects before another attack sets health or starts a spell.
    if(instruction->opCode==ECL_OPCODE_SPELLCARDSTART ||
       (instruction->opCode==ECL_OPCODE_ENEMYLIFESET && instruction->args.setInt>0)) {
        outroBlocked=true;enemy->flags.unk6=0;Settle(*enemy);return false;
    }
    return true;
}
Practice::Th06Run::SetupStep Practice::Th06Run::BeforeInstruction(Enemy* enemy,EclRawInstr* instruction) {
    if(!preparing)return SetupStep::Execute;
    const int sub=enemy->currentContext.subId,opcode=instruction->opCode;
    if(!graph.Contains(sub,instruction) || instruction->time<0 ||
       std::find(visited.begin(),visited.end(),instruction)!=visited.end() || visited.size()>g_EclManager.fileSize/12)return SetupStep::Stop;
    if(seekTime>=0 && instruction->time>=seekTime){enemy->currentContext.currentInstr=instruction;return SetupStep::Stop;}
    visited.push_back(instruction);enemy->currentContext.time.SetCurrent(instruction->time);
    if(seekTime>=0)return SetupStep::Skip;
    if(opcode==0 || opcode==1 || opcode==ECL_OPCODE_RET)return SetupStep::Stop;
    if(opcode==ECL_OPCODE_CALL || (opcode>=ECL_OPCODE_CALLLSS && opcode<=ECL_OPCODE_CALLNEQ) ||
       (opcode>=ECL_OPCODE_BULLETFANAIMED && opcode<=ECL_OPCODE_ENEMYKILLALL) ||
       opcode==ECL_OPCODE_EFFECTSOUND || opcode==ECL_OPCODE_ENEMYINTERRUPT || opcode==ECL_OPCODE_EFFECTPARTICLE || opcode==ECL_OPCODE_DROPITEMS)
        return SetupStep::Skip;
    return SetupStep::Execute;
}
int Practice::Th06Run::ShotRoute() {
    return routeOverride<0?g_GameManager.CharacterShotType():routeOverride;
}
bool Practice::Th06Run::QedRage() {
    return qedRage;
}

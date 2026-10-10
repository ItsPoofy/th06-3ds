#pragma once
struct AnmVm;
struct ZunVec2;
struct ZunVec3;
namespace CtrPlayfieldText {
    inline constexpr int BonusDuration=250;
    inline constexpr float Left=48.f,Right=400.f,Top=40.f,Bottom=440.f;
    void ResetDialoguePanel();
    void TickDialoguePanel(bool visible);
    float DialoguePanelOpacity();
    float DialogueTop();
    float ResultsLeft();
    float Scale();
    unsigned Fade(int frame,int duration);
    void QueueNotification(const ZunVec3& position,const char* text,int frame,int duration,int fadeOutFrame=-1);
    void CueMusic();
    void TickMusic(bool dialogueVisible);
    void Draw(const AnmVm& source, bool centered = false, float targetScale = 0);
    void DrawStageTitle(const AnmVm& source,int stage);
    void DrawMusicTitle(const AnmVm& source);
    void DrawSpellcard(const AnmVm& source,const AnmVm& background);
    void DrawDialogue(const AnmVm* lines);
    void QueueBonus(const ZunVec3& position,int score,int frames,int fadeOutFrame=-1);
    void EnlargeAscii(ZunVec3& position,ZunVec2& scale,const char* text,bool stageResults=false);
}

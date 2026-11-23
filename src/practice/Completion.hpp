#pragma once

namespace Practice::Completion {
enum class Action { None, Retry, ReturnPractice, Quit };
enum class Outcome { SectionCleared, SpellCleared };
inline constexpr int CountdownSeconds=10;
inline constexpr int SpellRepeatSeconds=2;
void Reset();
void Begin(bool repeat,Outcome outcome=Outcome::SectionCleared);
void BeginExit(Action action);
bool Active();
bool CountingDown();
bool Leaving();
void Choose(Action action);
Action AutomaticAction();
// Wait for the native outro, then count down before returning the chosen action.
Action Tick(bool outroFinished);
int SecondsRemaining();
float TextOpacity();
}

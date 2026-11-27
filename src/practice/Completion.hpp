#pragma once

namespace Practice::Completion {
enum class Action { None, Retry, ReturnPractice, Quit };
inline constexpr int CountdownSeconds=10;
void Reset();
void Begin(bool repeat);
void BeginExit(Action action);
bool Active();
bool CountingDown();
bool Leaving();
void Choose(Action action);
Action AutomaticAction();
Action Tick(bool outroFinished);
int SecondsRemaining();
float TextOpacity();
}

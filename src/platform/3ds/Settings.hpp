#pragma once
namespace CtrSettings {
enum class ScreenMode { Split, TopOnly };
enum class Toggle { Hitbox, Focus, Fire, QuickInfo, AutoCollect, Fps, BossBar };
enum class Content { Language, Graphics, Sound, Sprites };
bool English();
bool Retexture();
bool NewSprites();
int SoundPack();
const char* SoundPackName();
void ChangeContent(Content setting,int direction);
void Load();
void Save();
void Reset();
ScreenMode Mode();
void ChangeMode();
int BgmVolume();
int SfxVolume();
void ChangeVolume(bool bgm,int direction);
bool Enabled(Toggle setting);
void ChangeToggle(Toggle setting);
bool CollectAtTop(int power,float y);
void ReplayCollection(bool always);
void EndReplayCollection();
}

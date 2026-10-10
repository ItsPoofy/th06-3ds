#include "Settings.hpp"
#include <atomic>
#include <algorithm>
#include <cstdio>
namespace {
std::atomic_int bgm{100},sfx{100};
CtrSettings::ScreenMode mode=CtrSettings::ScreenMode::TopOnly;
bool toggles[]={true,false,false,true,false,true,true};
bool replayCollection=false,replayAlways=false;
unsigned char content[]={1,0,0,0};
}
void CtrSettings::Load() {
    if(FILE* f=fopen("settings3ds.cfg","rb")) {
        unsigned char data[18]{};
        const auto size=fread(data,1,sizeof(data),f);
        if(((size==10 && data[3]==1) || (size==11 && data[3]==2 && data[10]<2) ||
           (size==12 && data[3]==3 && data[10]<2 && data[11]<2) ||
           (size==13 && data[3]==4 && data[10]<2 && data[11]<2 && data[12]<2) ||
           (((size==17 && data[3]==5) || (size==18 && data[3]==6 && data[17]<2)) && data[10]<2 && data[11]<2 && data[12]<2 &&
            data[13]<2 && data[14]<2 && data[15]<4 && data[16]<2)) && data[0]=='T' && data[1]=='H' &&
           data[2]=='S' && data[4]<2 && data[5]<=100 && data[6]<=100 &&
           data[5]%10==0 && data[6]%10==0 && data[7]<2 && data[8]<2 && data[9]<2) {
            mode=static_cast<ScreenMode>(data[4]);bgm=data[5];sfx=data[6];
            for(int i=0;i<3;i++)toggles[i]=data[7+i];
            toggles[3]=size>=11?data[10]!=0:true;
            toggles[4]=size>=12?data[11]!=0:false;
            toggles[5]=size>=13?data[12]!=0:true;
            if(size>=17)for(int i=0;i<4;i++)content[i]=data[13+i];
            if(size==18)toggles[6]=data[17]!=0;
        }
        fclose(f);
    }
}
void CtrSettings::Save() {
    if(FILE* f=fopen("settings3ds.cfg","wb")) {
        const unsigned char data[]={'T','H','S',6,static_cast<unsigned char>(mode),
            static_cast<unsigned char>(bgm.load()),static_cast<unsigned char>(sfx.load()),
            toggles[0],toggles[1],toggles[2],toggles[3],toggles[4],toggles[5],content[0],content[1],content[2],content[3],toggles[6]};
        fwrite(data,1,sizeof(data),f);fclose(f);
    }
}
void CtrSettings::Reset() {mode=ScreenMode::TopOnly;bgm=100;sfx=100;toggles[0]=true;toggles[1]=toggles[2]=false;toggles[3]=true;toggles[4]=false;toggles[5]=toggles[6]=true;content[0]=1;content[1]=content[2]=content[3]=0;}
bool CtrSettings::English() {return content[0]!=0;}
bool CtrSettings::Retexture() {return content[1]!=0;}
bool CtrSettings::NewSprites() {return content[3]!=0;}
int CtrSettings::SoundPack() {return content[2];}
const char* CtrSettings::SoundPackName() {static const char* names[]={"Original","PCB","IN","SA"};return names[content[2]];}
void CtrSettings::ChangeContent(Content setting,int direction) {auto& value=content[static_cast<int>(setting)];const int count=setting==Content::Sound?4:2;value=(value+direction+count)%count;}
CtrSettings::ScreenMode CtrSettings::Mode() {return mode;}
void CtrSettings::ChangeMode() {mode=mode==ScreenMode::Split?ScreenMode::TopOnly:ScreenMode::Split;}
int CtrSettings::BgmVolume() {return bgm.load(std::memory_order_relaxed);}
int CtrSettings::SfxVolume() {return sfx.load(std::memory_order_relaxed);}
void CtrSettings::ChangeVolume(bool music,int direction) {
    auto& volume=music?bgm:sfx;volume=std::clamp(volume.load()+direction*10,0,100);
}
bool CtrSettings::Enabled(Toggle setting) {return toggles[static_cast<int>(setting)];}
void CtrSettings::ChangeToggle(Toggle setting) {auto& value=toggles[static_cast<int>(setting)];value=!value;}
bool CtrSettings::CollectAtTop(int power,float y) {return y<128.f && (power>=128 || (replayCollection?replayAlways:toggles[4]));}
void CtrSettings::ReplayCollection(bool always) {replayCollection=true;replayAlways=always;}
void CtrSettings::EndReplayCollection() {replayCollection=false;}

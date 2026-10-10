#include "DisplaySettings.hpp"
#include "Settings.hpp"
#include <cstdio>
namespace {
CtrDisplay::Alignment alignment=CtrDisplay::Alignment::Bottom;
void Save() {
    if(FILE* f=fopen("display.cfg","wb")) {
        const unsigned char data[]={ 'T','H','D',1,static_cast<unsigned char>(alignment) };
        fwrite(data,1,sizeof(data),f);fclose(f);
    }
}
}
void CtrDisplay::Load() {
    if(FILE* f=fopen("display.cfg","rb")) {
        unsigned char data[5]{};
        if(fread(data,1,5,f)==5 && data[0]=='T' && data[1]=='H' && data[2]=='D' && data[3]==1 && data[4]<3)
            alignment=static_cast<Alignment>(data[4]);
        fclose(f);
    }
}
void CtrDisplay::Cycle(int direction) {
    if(CtrSettings::Mode()==CtrSettings::ScreenMode::TopOnly)return;
    alignment=static_cast<Alignment>((static_cast<int>(alignment)+direction+3)%3);Save();
}
void CtrDisplay::Reset() {alignment=Alignment::Bottom;Save();}
CtrDisplay::Alignment CtrDisplay::GetAlignment() {return CtrSettings::Mode()==CtrSettings::ScreenMode::TopOnly?Alignment::Top:alignment;}
const char* CtrDisplay::AlignmentName() {
    static const char* names[]={"Top","Middle","Bottom"};return names[static_cast<int>(GetAlignment())];
}
float CtrDisplay::PlayfieldOffset() {
    return (480.0f-448.0f*5.0f/6.0f)*static_cast<int>(GetAlignment())/2.0f;
}

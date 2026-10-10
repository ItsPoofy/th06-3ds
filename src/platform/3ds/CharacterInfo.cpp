#include "CharacterInfo.hpp"
#include "SelectionArt.hpp"
#include <citro2d.h>
#include <cstdio>
namespace {
void Label(const char* screen,const char* field,int character,float centerX,float y,unsigned opacity) {
    char label[40];
    snprintf(label,sizeof(label),"character-%s-%s%d",screen,field,character);
    CtrSelectionArt::Draw(label,centerX,y,C2D_Color32(255,255,255,opacity),true);
}
}
void CtrCharacterInfo::DrawIdentity(int character,float centerX,float y,unsigned opacity) {
    Label("bottom","title",character,centerX,y,opacity);
    Label("bottom","name",character,centerX,y+24,opacity);
}
void CtrCharacterInfo::DrawFull(int character,float centerX,float y,unsigned opacity) {
    Label("top","title",character,centerX,y,opacity);
    Label("top","name",character,centerX,y+26,opacity);
    Label("top","specs",character,centerX,y+54,opacity);
}

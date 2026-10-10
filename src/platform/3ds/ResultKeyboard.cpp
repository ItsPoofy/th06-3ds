#include "ResultKeyboard.hpp"
#include "CtrRenderer.hpp"
#include "GameLoop.hpp"
#include "Touch.hpp"
#include "Controller.hpp"
#include "Supervisor.hpp"
#include <3ds.h>
#include <cstring>
namespace {
SwkbdCallbackResult ValidateName(void*,const char** message,const char* text,size_t length) {
    // Score and replay files store eight single-byte glyphs, not UTF-8 names.
    for(size_t i=0;i<length && text[i];i++)if(static_cast<unsigned char>(text[i])<32 || static_cast<unsigned char>(text[i])>126) {
        *message="Use letters, numbers or standard symbols.";
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}
}
bool CtrKeyboard::InputName(char (&name)[9],const char* hint) {
    char text[9]{};std::memcpy(text,name,8);
    for(int i=7;i>=0 && text[i]==' ';--i)text[i]=0;
    SwkbdState keyboard;
    swkbdInit(&keyboard,SWKBD_TYPE_WESTERN,2,8);
    swkbdSetFeatures(&keyboard,SWKBD_DARKEN_TOP_SCREEN|SWKBD_DEFAULT_QWERTY);
    swkbdSetHintText(&keyboard,hint);
    swkbdSetInitialText(&keyboard,text);
    swkbdSetButton(&keyboard,SWKBD_BUTTON_LEFT,"Cancel",false);
    swkbdSetButton(&keyboard,SWKBD_BUTTON_RIGHT,"OK",true);
    swkbdSetValidation(&keyboard,SWKBD_ANYTHING,SWKBD_FILTER_CALLBACK,0);
    swkbdSetFilterCallback(&keyboard,ValidateName,nullptr);
    auto* renderer=static_cast<CtrRenderer*>(g_GfxBackend);
    renderer->SuspendForApplet();
    const auto button=swkbdInputText(&keyboard,text,sizeof(text));
    renderer->ResumeFromApplet();
    CtrGameLoop::ResetClock();
    hidScanInput();CtrTouch::Scan();int x,y;CtrTouch::Tap(x,y);
    g_CurFrameInput=g_LastFrameInput=Controller::GetInput();
    g_IsEigthFrameOfHeldInput=g_NumOfFramesInputsWereHeld=0;
    if(button!=SWKBD_BUTTON_CONFIRM)return false;
    std::memset(name,' ',8);std::memcpy(name,text,std::strlen(text));name[8]=0;
    return true;
}

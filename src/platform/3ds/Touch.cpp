#include "Touch.hpp"
#include <3ds.h>
namespace { bool wasHeld=false,pending=false; touchPosition position{}; }
void CtrTouch::Scan() {
    const bool held=(hidKeysHeld()&KEY_TOUCH)!=0;
    pending=held && !wasHeld;wasHeld=held;
    if(pending)hidTouchRead(&position);
}
bool CtrTouch::Tap(int& x,int& y) {
    if(!pending)return false;
    pending=false;x=position.px;y=position.py;return true;
}

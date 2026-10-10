#include "ExtraRecordsCode.hpp"
#include <3ds.h>
namespace CtrExtraRecords {
int Update(i32& step) {
    const u32 held=hidKeysHeld();
    if(!(held&(KEY_L|KEY_R))) {step=0;return 0;}
    const u32 pressed=hidKeysDown()&~(KEY_L|KEY_R);
    if(!pressed)return 1;
    const u32 expected=step<5?KEY_START:step<7?KEY_A:KEY_B;
    if(pressed!=expected) {step=pressed==KEY_START?1:0;return 1;}
    if(++step==10) {step=0;return 2;}
    return 1;
}
}

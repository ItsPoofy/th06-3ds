#pragma once
#include <citro2d.h>
// Transparent native-sized layer for the engine's difficulty UI.
class CtrMenuLayer {
public:
    bool Initialize();
    void Shutdown();
 void Begin(bool clear=true);
    void Reset() {visible=false;}
    void Draw(C3D_RenderTarget* bottom);
private:
    C3D_Tex texture{};
    C3D_RenderTarget* target=nullptr;
    bool ready=false,visible=false;
};

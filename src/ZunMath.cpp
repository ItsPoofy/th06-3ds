


#include "ZunMath.hpp"
#include "GameWindow.hpp"
#ifdef __3DS__
#include "AnmManager.hpp"
#endif

void ZunViewport::Set() const
{
#ifdef __3DS__
    u32 oldViewport[4];
    g_GfxBackend->GetViewport(oldViewport);
    const bool changed=oldViewport[0]!=this->x ||
        oldViewport[1]!=GAME_WINDOW_HEIGHT-(this->y+this->height) ||
        oldViewport[2]!=this->width || oldViewport[3]!=this->height;
    if(changed && g_AnmManager)g_AnmManager->FlushVertexBuffer();
#endif
    g_GfxBackend->SetViewport(this->x * WIDTH_RESOLUTION_SCALE + VIEWPORT_OFF_X,
                              (GAME_WINDOW_HEIGHT_REAL - ((this->y + this->height) * HEIGHT_RESOLUTION_SCALE)) -
                                  VIEWPORT_OFF_Y,
                              this->width * WIDTH_RESOLUTION_SCALE, this->height * HEIGHT_RESOLUTION_SCALE);
    g_GfxBackend->SetDepthRange(this->minZ, this->maxZ);
}

void ZunViewport::Get()
{
    u32 viewPortGet[4];
    f32 depthRangeGet[2];

    g_GfxBackend->GetViewport(viewPortGet);
    g_GfxBackend->GetDepthRange(depthRangeGet);

    this->x = (viewPortGet[0] - VIEWPORT_OFF_X) / WIDTH_RESOLUTION_SCALE;
    this->y = (viewPortGet[1] - VIEWPORT_OFF_Y) / HEIGHT_RESOLUTION_SCALE;
    this->width = viewPortGet[2] / WIDTH_RESOLUTION_SCALE;
    this->height = viewPortGet[3] / HEIGHT_RESOLUTION_SCALE;
    this->minZ = depthRangeGet[0];
    this->maxZ = depthRangeGet[1];

    // Convert from OpenGL to D3D conventions
    this->y = GAME_WINDOW_HEIGHT - (this->y + this->height);
}


ZunMatrix inverseViewportMatrix()
{
    ZunMatrix inverseMatrix;
    ZunViewport viewport;

    viewport.Get();

    inverseMatrix.Identity();



    inverseMatrix.Translate(-1.0f, 1.0f, -1.0f);
    inverseMatrix.Scale(1.0f / (viewport.width / 2.0f), -1.0f / (viewport.height / 2.0f), 2.0f);
    inverseMatrix.Translate(-viewport.x, -viewport.y, 0.0f);

    g_GfxBackend->SetDepthRange(0.0f, 1.0f);

    return inverseMatrix;
}

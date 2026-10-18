#pragma once
#include "graphics/GfxInterface.hpp"
#include "MenuLayer.hpp"
#include "SpriteVertex.hpp"
#include <3ds.h>
#include <citro3d.h>
#include <memory>
#include <vector>

// PICA200 implementation of the portable engine's state-based renderer.
class CtrRenderer final : public GfxInterface {
public:
    static GfxInterface* Init();
    ~CtrRenderer() override;
    void SetFogRange(f32 n, f32 f) override { fogDirty|=fogNear!=n || fogFar!=f; fogNear=n; fogFar=f; }
    void SetFogColor(ZunColor c) override { fogColor=c; }
    void ToggleVertexAttribute(u8 attr,bool on) override;
    void SetAttributePointer(VertexAttributeArrays attr,std::size_t stride,void* p) override;
    void SetColorOp(TextureOpComponent c,ColorOp op) override { colorOps[c]=op; }
    void SetTextureFactor(ZunColor c) override { factor=c; }
    void SetTransformMatrix(TransformMatrix t,const ZunMatrix& m) override {
        const bool changed=std::memcmp(&matrices[t],&m,sizeof(m))!=0;
        if(t==MATRIX_PROJECTION && changed) fogDirty=true;
        if(changed) { if(t==MATRIX_TEXTURE)textureMatrixDirty=true;else clipMatrixDirty=true; }
        matrices[t]=m;
    }
    void SetTextureFilter() override;
    void GetViewport(u32* v) override;
    void GetDepthRange(f32* v) override { v[0]=depthNear; v[1]=depthFar; }
    void SetViewport(i32 x,i32 y,i32 w,i32 h) override;
    void SetDepthRange(f32 n,f32 f) override { fogDirty|=depthNear!=n || depthFar!=f; depthNear=n; depthFar=f; }
    void Enable(Capabilities c) override { if(c==CAPS_DEPTH_TEST) depthTest=true; }
    bool HasError() override { bool e=error; error=false; return e; }
    void SetBlendMode(BlendMode m) override { blend=m; }
    void SetDepthMask(bool e) override { depthMask=e; }
    void SetDepthFunc(DepthFunc f) override { depthFunc=f; }
    void SetClearDepth(f32 d) override { clearDepth=d; }
    void SetClearColor(f32 r,f32 g,f32 b,f32 a) override;
    void Clear(u32 bits) override;
    GfxTextureHandle CreateTexture() override;
    void BindTexture(GfxTextureHandle h) override;
    void DeleteTexture(GfxTextureHandle h) override;
    void SetTextureImage(u32 w,u32 h,PixelFormat f,PixelDataType t,const void* data) override;
    void SetTextureSubImage(i32 x,i32 y,i32 w,i32 h,const void* data) override;
    void ReadPixels(i32 x,i32 y,i32 w,i32 h,const void* p) override;
    void Draw(PrimitiveType t,i32 first,i32 count) override;
    CtrGraphics::SpriteVertex* ReserveSpriteVertices(u32 count);
    void SwapBuffers() override;
    GfxTextureHandle LoadPreparedTexture(const char* path,u32& width,u32& height);
    void UpdateTextRows(i32 y,i32 width,i32 height,const u8* rgba);
    C3D_Tex* TextureImage(GfxTextureHandle handle);
    void SetGameplayLayout(bool playing) { gameplayLayout=playing; }
    void CompleteScene() { sceneComplete=true; }
    void InvalidateScene() { sceneComplete=false; }
    void BeginMenuLayer(bool clear=true);
    void EndMenuLayer();
    void ShowLoadingScreen();
    void SuspendForApplet();
    void ResumeFromApplet();
    void PrepareForExit();
    void SynchronizeAssets();
    void SetStageDepthBias(float bias) {
        if(stageDepthBias!=bias) {stageDepthBias=bias;clipMatrixDirty=true;}
    }
private:
    struct Texture {
        C3D_Tex gpu{};
        bool ready=false;
        ~Texture() { if(ready) C3D_TexDelete(&gpu); }
    };
    using Vertex=CtrGraphics::SpriteVertex;
    static_assert(sizeof(Vertex)==24);
    static void OnLifecycle(APT_HookType type,void* context);
    aptHookCookie lifecycleHook{};
    bool lifecycleHooked=false,suspended=false,exiting=false;
    bool Start();
    void BeginFrame();
    void ApplyState();
    void ApplyFog();
    void FlushVertices();
    void EndSegment();
    void SplitSegment();
    void FinishTextureReads();
    void UploadPixel(Texture& tex,int x,int y,const u8* p,PixelFormat f,PixelDataType t);
    CtrMenuLayer menuLayer;
    bool drawingMenuLayer=false;
    u32 savedViewport[4]{};
    C3D_Tex scene{};
    struct FogTable {float key[8]{};C3D_FogLut lut{};bool valid=false;};
    FogTable fogTables[4];
    unsigned nextFogTable=0;
    bool fogDirty=true;
    C3D_RenderTarget *sceneTarget=nullptr,*top=nullptr,*bottom=nullptr;
    DVLB_s* shader=nullptr;
    shaderProgram_s program{};
    Vertex* vertices=nullptr;
    static constexpr u32 MaxVertices=131072;
    u32 vertexCursor=0;
    unsigned queuedCommands=0;
    bool frameOpen=false,error=false,started=false;
    bool gameplayLayout=false,sceneComplete=false;
    std::vector<std::unique_ptr<Texture>> textures{1};
    std::vector<std::unique_ptr<Texture>> retired;
    Texture* bound=nullptr;
    ZunMatrix matrices[4];
    ZunMatrix clipMatrix;
    struct DrawState {
        bool valid=false,textured=false,menu=false,depthTest=false,depthMask=false,fogEnabled=false;
        u32 viewport[4]{};
        float depthNear=0,depthFar=1;
        DepthFunc depthFunc=DEPTH_FUNC_LEQUAL;
        BlendMode blend=BLEND_INV_SRC_ALPHA;
        ColorOp colorOps[2]{};
        C3D_Tex* texture=nullptr;
        ZunColor fogColor=0;
    } applied;
    bool clipMatrixDirty=true;
    float stageDepthBias=0;
    bool textureMatrixDirty=true;
    int clipUniform=-1,textureUniform=-1;
    void* pointers[3]{};
    std::size_t strides[3]{};
    bool attributes[2]{true,false},depthTest=false,depthMask=true;
    ColorOp colorOps[2]{COLOR_OP_MODULATE,COLOR_OP_MODULATE};
    ZunColor factor=0xffffffff,fogColor=0;
    float fogNear=0,fogFar=1000,depthNear=0,depthFar=1,clearDepth=1;
    u32 viewport[4]{0,0,640,480},clearColor=0x000000ff;
    BlendMode blend=BLEND_INV_SRC_ALPHA;
    DepthFunc depthFunc=DEPTH_FUNC_LEQUAL;
};

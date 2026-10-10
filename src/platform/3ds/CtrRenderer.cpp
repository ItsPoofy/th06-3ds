#include "CtrRenderer.hpp"
#include "Runtime.hpp"
#include "Presentation.hpp"
#include "DisplaySettings.hpp"
#include "GameplayHud.hpp"
#include "GameplayLayout.hpp"
#include "GameDecorations.hpp"
#include "BossHealthBar.hpp"
#include "SelectionLayout.hpp"
#include "PauseMenu.hpp"
#include "Supervisor.hpp"
#include "AssetPath.hpp"
#include "scene_shader.hpp"
#include <citro2d.h>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace {
// PICA textures use 8x8 Morton tiles and ABGR bytes.
u32 TexelOffset(u32 x,u32 y,u32 width) {
    u32 m=0;
    for(u32 b=0;b<3;b++) m|=((x>>b)&1)<<(2*b) | ((y>>b)&1)<<(2*b+1);
    return ((y>>3)*(width>>3)+(x>>3))*64+m;
}
u32 ABGR(ZunColor c) { return (c&0xff00ff00)|((c&255)<<16)|((c>>16)&255); }
void NativeBlend(bool premultiplied=false) {
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,premultiplied?GPU_ONE:GPU_SRC_ALPHA,
        GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ONE_MINUS_SRC_ALPHA);
}
}
GfxInterface* CtrRenderer::Init() {
    auto* r=new CtrRenderer;
    if(!r->Start()) { delete r; return nullptr; }
    return r;
}
bool CtrRenderer::Start() {
    CtrRuntime::Log("Renderer: gfxInit");
    gfxInitDefault();
    gfxSet3D(false);
    if(!C3D_Init(2*1024*1024)) {CtrRuntime::Log("Renderer: C3D_Init failed");return false;}
    CtrRuntime::Log("Renderer: C3D ready");
    started=true;
    if(!C2D_Init(256)) {CtrRuntime::Log("Renderer: C2D_Init failed");return false;}
    top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
    bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
    if(!C3D_TexInitVRAM(&scene,1024,512,GPU_RGBA8)) {CtrRuntime::Log("Renderer: scene texture failed");return false;}
    CtrRuntime::Log("Renderer: texture ready");
    C3D_TexSetFilter(&scene,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&scene,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    sceneTarget=C3D_RenderTargetCreateFromTex(&scene,GPU_TEXFACE_2D,0,GPU_RB_DEPTH24);
    shader=DVLB_ParseFile((u32*)scene_shader,sizeof(scene_shader));
    if(!shader || !sceneTarget || !top || !bottom) {CtrRuntime::Log("Renderer: shader or target failed");return false;}
    CtrRuntime::Log("Renderer: targets ready");
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program,&shader->DVLE[0]);
    clipUniform=shaderInstanceGetUniformLocation(program.vertexShader,"clipRows");
    textureUniform=shaderInstanceGetUniformLocation(program.vertexShader,"textureRows");
    if(clipUniform<0 || textureUniform<0)return false;
    vertices=(Vertex*)linearAlloc(sizeof(Vertex)*MaxVertices);
    if(!vertices) {CtrRuntime::Log("Renderer: vertex allocation failed");return false;}
    if(!menuLayer.Initialize()) {CtrRuntime::Log("Renderer: menu layer failed");return false;}
    if(!CtrPresentation::Initialize()) {CtrRuntime::Log("Renderer: UI assets failed");return false;}
    if(!CtrBossHealthBar::Initialize(*this)) {CtrRuntime::Log("Renderer: boss bar texture failed");return false;}
    for(auto& m:matrices) m.Identity();
    // Run before citro3d's hook so pending geometry is submitted before
    // its suspend handler drains the GPU queue and disables VBlank callbacks.
    aptHook(&lifecycleHook,OnLifecycle,this);lifecycleHooked=true;
    BeginFrame();
    CtrRuntime::Log("Renderer initialized");
    return true;
}
void CtrRenderer::OnLifecycle(APT_HookType type,void* context) {
    auto& renderer=*static_cast<CtrRenderer*>(context);
    if(type==APTHOOK_ONSUSPEND) {
        if(renderer.frameOpen) {renderer.FlushVertices();renderer.EndSegment();}
        renderer.suspended=true;
    } else if(type==APTHOOK_ONRESTORE) {
        renderer.suspended=false;renderer.applied.valid=false;
        // Begin the next frame after all SDK restore hooks have completed.
    }
}
void CtrRenderer::PrepareForExit() {
    if(exiting)return;
    exiting=true;
    if(frameOpen) {FlushVertices();EndSegment();}
    if(started && !suspended) {
        // Wait for GPU reads without relying on VBlank. A cancelled app may
        // never restore those callbacks; the suspend hook already drained it.
        if(C3D_FrameBegin(0))C3D_FrameEnd(0);
    }
}
CtrRenderer::~CtrRenderer() {
    PrepareForExit();
    if(lifecycleHooked)aptUnhook(&lifecycleHook);
    CtrBossHealthBar::Shutdown(*this);
    textures.clear(); retired.clear();
    CtrPresentation::Shutdown();
    menuLayer.Shutdown();
    if(vertices) linearFree(vertices);
    if(shader) { shaderProgramFree(&program); DVLB_Free(shader); }
    if(sceneTarget) C3D_RenderTargetDelete(sceneTarget);
    if(scene.data) C3D_TexDelete(&scene);
    if(top) C3D_RenderTargetDelete(top);
    if(bottom) C3D_RenderTargetDelete(bottom);
    if(started) { C2D_Fini(); C3D_Fini(); }
    gfxExit();
}
void CtrRenderer::BeginFrame() {
    if(frameOpen || exiting || suspended || aptShouldClose())return;
    if(!C3D_FrameBegin(C3D_FRAME_SYNCDRAW))return;
    frameOpen=true; sceneComplete=false; menuLayer.Reset(); vertexCursor=0; queuedCommands=0; retired.clear();
    C3D_FrameDrawOn(sceneTarget);
    ApplyState();
}
void CtrRenderer::ApplyState() {
    // citro2d, target changes and resumed frames invalidate all cached bindings.
    applied.valid=false;
    C3D_BindProgram(&program);
    clipMatrixDirty=textureMatrixDirty=true;
    C3D_AttrInfo* a=C3D_GetAttrInfo(); AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(a,1,GPU_FLOAT,2);
    AttrInfo_AddLoader(a,2,GPU_UNSIGNED_BYTE,4);
    C3D_BufInfo* bi=C3D_GetBufInfo();BufInfo_Init(bi);
    if(vertices)BufInfo_Add(bi,vertices,sizeof(Vertex),3,0x210);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);
    C3D_AlphaTest(true,GPU_GEQUAL,4);
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_TexBind(0,nullptr);
    for(int i=0;i<6;i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
}
void CtrRenderer::ToggleVertexAttribute(u8 a,bool on) {
    if(a&VERTEX_ATTR_TEX_COORD) attributes[0]=on;
    if(a&VERTEX_ATTR_DIFFUSE) attributes[1]=on;
}
void CtrRenderer::SetAttributePointer(VertexAttributeArrays a,std::size_t stride,void* p) {
    pointers[a]=p; strides[a]=stride;
}
void CtrRenderer::GetViewport(u32* v) {
    // Engine RHW vertices remain in 640x480 coordinates on the native layer.
    // Report the logical viewport so inverseViewportMatrix does not scale twice.
    for(int i=0;i<4;i++)v[i]=viewport[i]*(drawingMenuLayer?2:1);
}
void CtrRenderer::SetViewport(i32 x,i32 y,i32 w,i32 h) {
    if(drawingMenuLayer) {x/=2;y/=2;w/=2;h/=2;}
    viewport[0]=x; viewport[1]=y; viewport[2]=w; viewport[3]=h;
}
void CtrRenderer::SetClearColor(f32 r,f32 g,f32 b,f32 a) {
    // RGBA8 framebuffer memory is A,B,G,R, matching tiled texture memory.
    clearColor=(u32(r*255)<<24)|(u32(g*255)<<16)|(u32(b*255)<<8)|u32(a*255);
}
void CtrRenderer::FinishTextureReads() {
    if(exiting || suspended)return;
    if(retired.empty() && !vertexCursor && !queuedCommands) return;
    // Finish geometry before reclaiming textures; retain the scene and displays.
    if(frameOpen) EndSegment();
    C3D_FrameBegin(0);frameOpen=true;
    retired.clear();vertexCursor=0;queuedCommands=0;
    if(drawingMenuLayer)menuLayer.Begin(false);else C3D_FrameDrawOn(sceneTarget);
    ApplyState();
}
void CtrRenderer::SynchronizeAssets() {
    if(exiting || suspended)return;
    if(sceneComplete)SwapBuffers();
    // FrameBegin waits for the submitted GPU queue, including native C2D
    // artwork; option changes must not free an atlas still being sampled.
    C2D_Flush();
    if(frameOpen)EndSegment();
    C3D_FrameBegin(0);frameOpen=true;
    retired.clear();vertexCursor=0;queuedCommands=0;
    if(drawingMenuLayer)menuLayer.Begin(false);else C3D_FrameDrawOn(sceneTarget);
    ApplyState();
}
void CtrRenderer::UpdateTextRows(i32 y,i32 width,i32 height,const u8* rgba) {
    if(!bound || !bound->ready) return;
    FinishTextureReads();
    for(int row=0;row<height;row++) for(int x=0;x<width;x++)
        UploadPixel(*bound,x,y+row,rgba+(row*width+x)*4,PIXEL_RGBA,PIXEL_UNSIGNED_BYTE);
    C3D_TexFlush(&bound->gpu);
}
GfxTextureHandle CtrRenderer::LoadPreparedTexture(const char* path,u32& width,u32& height) {
    // A draw pass may be followed by asset loading inside an update callback.
    // Commit its captured layout/menu before textures or backgrounds change.
    if(sceneComplete) SwapBuffers();
    const auto filename=CtrAssets::PackedPath("textures",(std::string(path)+".bin").c_str());
    FILE* file=fopen(filename.c_str(),"rb");
    if(!file) {CtrRuntime::Log(("Missing prepared texture: "+filename).c_str());return {};}
    u32 header[5];
    if(fread(header,sizeof(header),1,file)!=1 || header[0]!=0x44335854 ||
       header[3]<8 || header[4]<8 || header[3]>1024 || header[4]>1024 ||
       header[1]>header[3] || header[2]>header[4]) {fclose(file);return {};}
    FinishTextureReads();
    const auto handle=CreateTexture();auto& texture=*textures[handle.id];
    if(!C3D_TexInit(&texture.gpu,header[3],header[4],GPU_RGBA8)) {
        char message[256];snprintf(message,sizeof(message),"Texture allocation failed: %s (%lux%lu), linear free %lu KiB",filename.c_str(),static_cast<unsigned long>(header[3]),static_cast<unsigned long>(header[4]),static_cast<unsigned long>(linearSpaceFree()/1024));CtrRuntime::Log(message);
        textures[handle.id].reset();fclose(file);return {};
    }
    texture.ready=true;
    const bool loaded=fread(texture.gpu.data,1,texture.gpu.size,file)==texture.gpu.size;
    fclose(file);
    if(!loaded) {textures[handle.id].reset();return {};}
    C3D_TexSetWrap(&texture.gpu,GPU_REPEAT,GPU_REPEAT);
    C3D_TexSetFilter(&texture.gpu,GPU_LINEAR,GPU_LINEAR);C3D_TexFlush(&texture.gpu);
    width=header[1];height=header[2];return handle;
}
void CtrRenderer::ApplyFog() {
    const bool enabled=!(g_Supervisor.cfg.opts & (1<<GCOS_DONT_USE_FOG));
    if(!applied.valid || applied.fogEnabled!=enabled)
        C3D_FogGasMode(enabled?GPU_FOG:GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    applied.fogEnabled=enabled;
    if(!enabled)return;
    if(fogDirty) {
        const auto& p=matrices[MATRIX_PROJECTION];
        const float key[]={p.m[2][3],p.m[2][2],p.m[3][2],p.m[3][3],depthNear,depthFar,fogNear,fogFar};
        FogTable* table=nullptr;
        // Switching between stage perspective and 2D sprites does not change
        // their fog tables. Reuse them instead of doing 129 divisions again.
        for(auto& candidate:fogTables)if(candidate.valid && std::memcmp(candidate.key,key,sizeof(key))==0) {
            table=&candidate;break;
        }
        if(!table) {
            table=&fogTables[nextFogTable++%4];
            float data[256],previous=0;
            for(int i=0;i<=128;i++) {
                const float depth=i/128.0f;
                const float ndc=2*(depth-depthNear)/(depthFar-depthNear)-1;
                const float denominator=ndc*p.m[2][3]-p.m[2][2];
                const float viewZ=std::abs((p.m[3][2]-ndc*p.m[3][3])/denominator);
                const float value=std::clamp((fogFar-viewZ)/(fogFar-fogNear),0.0f,1.0f);
                if(i<128) data[i]=value;
                if(i>0) data[i+127]=value-previous;
                previous=value;
            }
            FogLut_FromArray(&table->lut,data);
            std::memcpy(table->key,key,sizeof(key));table->valid=true;
        }
        C3D_FogLutBind(&table->lut);fogDirty=false;
    }
    if(!applied.valid || applied.fogColor!=fogColor)C3D_FogColor(ABGR(fogColor)&0xffffff);
    applied.fogColor=fogColor;
}
void CtrRenderer::Clear(u32 bits) {
    if(!frameOpen)BeginFrame();
    if(!frameOpen)return;
    // libctru retains submitted entries until FrameBegin drains the queue.
    // Loading can run many clears without presenting a completed scene.
    // Leave room for the final draw list and both display transfers.
    if(queuedCommands>=16)FinishTextureReads();
    // GX memory fills are queued immediately, while draw commands normally
    // wait until FrameEnd. Submit pending draws before any engine clear so
    // the fill cannot overtake geometry that the engine already drew.
    if(vertexCursor) {
        FlushVertices();
        SplitSegment();
    }
    C3D_RenderTargetClear(sceneTarget,(C3D_ClearBits)bits,clearColor,
                         u32(std::clamp(clearDepth,0.0f,1.0f)*0xffffff));
    ++queuedCommands;
}
GfxTextureHandle CtrRenderer::CreateTexture() {
    for(u32 i=1;i<textures.size();i++) if(!textures[i]) {
        textures[i]=std::make_unique<Texture>(); return {i};
    }
    textures.push_back(std::make_unique<Texture>()); return {u32(textures.size()-1)};
}
void CtrRenderer::BindTexture(GfxTextureHandle h) {
    bound=(h.id<textures.size())?textures[h.id].get():nullptr;
}
void CtrRenderer::DeleteTexture(GfxTextureHandle h) {
    if(!h.id || h.id>=textures.size() || !textures[h.id]) return;
    if(bound==textures[h.id].get()) bound=nullptr;
    // The GPU may still be reading a temporary surface after Draw returns.
    retired.push_back(std::move(textures[h.id]));
}
void CtrRenderer::SetTextureFilter() {
    if(bound && bound->ready) {
        C3D_TexSetFilter(&bound->gpu,GPU_LINEAR,GPU_LINEAR);
        applied.texture=nullptr;
    }
}
void CtrRenderer::UploadPixel(Texture& tex,int x,int y,const u8* p,PixelFormat f,PixelDataType t) {
    u8 r=0,g=0,b=0,a=255;
    if(p && t==PIXEL_UNSIGNED_BYTE) { r=p[0];g=p[1];b=p[2];if(f==PIXEL_RGBA)a=p[3]; }
    else if(p) {
        u16 v;std::memcpy(&v,p,2);
        if(t==PIXEL_UNSIGNED_SHORT_5_6_5) {r=((v>>11)&31)*255/31;g=((v>>5)&63)*255/63;b=(v&31)*255/31;}
        else if(t==PIXEL_UNSIGNED_SHORT_5_5_5_1) {r=((v>>11)&31)*255/31;g=((v>>6)&31)*255/31;b=((v>>1)&31)*255/31;a=(v&1)*255;}
        else {r=((v>>12)&15)*17;g=((v>>8)&15)*17;b=((v>>4)&15)*17;a=(v&15)*17;}
    }
    u8* dst=(u8*)tex.gpu.data+TexelOffset(x,y,tex.gpu.width)*4;
    dst[0]=a;dst[1]=b;dst[2]=g;dst[3]=r;
}
void CtrRenderer::SetTextureImage(u32 w,u32 h,PixelFormat f,PixelDataType t,const void* data) {
    if(!bound) { error=true;return; }
    // Flush before replacing a texture referenced by already submitted geometry.
    if(bound->ready) {
        auto old=std::make_unique<Texture>();
        old->gpu=bound->gpu;old->ready=true;retired.push_back(std::move(old));bound->ready=false;
    }
    FinishTextureReads();
    if(!C3D_TexInit(&bound->gpu,std::max<u32>(8,BitCeil(w)),std::max<u32>(8,BitCeil(h)),GPU_RGBA8)) {error=true;return;}
    bound->ready=true;
    std::memset(bound->gpu.data,0,bound->gpu.size);
    C3D_TexSetWrap(&bound->gpu,GPU_REPEAT,GPU_REPEAT);
    C3D_TexSetFilter(&bound->gpu,GPU_LINEAR,GPU_LINEAR);
    const u32 bytes=t==PIXEL_UNSIGNED_BYTE?(f==PIXEL_RGB?3:4):2;
    if(data) for(u32 y=0;y<h;y++) for(u32 x=0;x<w;x++)
        UploadPixel(*bound,x,y,(const u8*)data+(y*w+x)*bytes,f,t);
    C3D_TexFlush(&bound->gpu);
}
void CtrRenderer::SetTextureSubImage(i32 x,i32 y,i32 w,i32 h,const void* data) {
    if(!bound || !bound->ready || !data) return;
    FinishTextureReads();
    for(int j=0;j<h;j++) for(int i=0;i<w;i++)
        if(x+i>=0 && y+j>=0 && x+i<bound->gpu.width && y+j<bound->gpu.height)
            UploadPixel(*bound,x+i,y+j,(const u8*)data+(j*w+i)*3,PIXEL_RGB,PIXEL_UNSIGNED_BYTE);
    C3D_TexFlush(&bound->gpu);
}
void CtrRenderer::ReadPixels(i32 x,i32 y,i32 w,i32 h,const void* p) {
    if(exiting || suspended || aptShouldClose())return;
    // Finish submitted rendering so bomb snapshots contain this frame's scene.
    if(frameOpen) EndSegment();
    // FrameBegin waits for the submitted GPU queue, unlike FrameSync (VBlank only).
    C3D_FrameBegin(0);frameOpen=true;vertexCursor=0;queuedCommands=0;
    GSPGPU_InvalidateDataCache(scene.data,scene.size);
    auto* dst=(u8*)p;
    for(int j=0;j<h;j++) for(int i=0;i<w;i++) {
        u8* d=dst+(j*w+i)*4;
        if(x+i<0 || y+j<0 || x+i>=640 || y+j>=480) {std::memset(d,0,4);continue;}
        const u8* s=(u8*)scene.data+TexelOffset(x+i,scene.height-1-(y+j),scene.width)*4;
        d[0]=s[3];d[1]=s[2];d[2]=s[1];d[3]=s[0];
    }
    C3D_FrameDrawOn(sceneTarget);ApplyState();
}
CtrGraphics::SpriteVertex* CtrRenderer::ReserveSpriteVertices(u32 count) {
    if(!frameOpen)BeginFrame();
    if(!frameOpen)return nullptr;
    if(count>MaxVertices-vertexCursor) {error=true;return nullptr;}
    auto* allocation=vertices+vertexCursor;
    vertexCursor+=count;
    return allocation;
}
void CtrRenderer::Draw(PrimitiveType type,i32 first,i32 count) {
    if(!frameOpen)BeginFrame();
    if(!frameOpen)return;
    if(count<3 || !pointers[VERTEX_ARRAY_POSITION]) return;
    const bool vertexColors=attributes[1] && pointers[2];
    const auto address=reinterpret_cast<std::uintptr_t>(pointers[0]);
    const auto base=reinterpret_cast<std::uintptr_t>(vertices);
    const auto offset=address-base;
    const bool inArena=address>=base && offset%sizeof(Vertex)==0 &&
        offset/sizeof(Vertex)+count<=vertexCursor;
    const auto* source=reinterpret_cast<const Vertex*>(pointers[0]);
    // Reserved sprite batches remain intact even if an immediate draw follows.
    const bool direct=inArena && first==0 && vertexColors && attributes[0] &&
        pointers[1]==&source->textureUV && pointers[2]==&source->diffuse &&
        strides[0]==sizeof(Vertex) && strides[1]==sizeof(Vertex) && strides[2]==sizeof(Vertex);
    if(!direct && u32(count)>MaxVertices-vertexCursor) {error=true;return;}
    if(clipMatrixDirty) {
        clipMatrix=matrices[MATRIX_PROJECTION]*(matrices[MATRIX_VIEW]*matrices[MATRIX_MODEL]);
        for(int row=0;row<4;row++) {
            float values[4];
            for(int col=0;col<4;col++)
                values[col]=row==2?(clipMatrix.m[col][2]-clipMatrix.m[col][3])*.5f:clipMatrix.m[col][row];
            C3D_FVUnifSet(GPU_VERTEX_SHADER,clipUniform+row,values[0],values[1],values[2],values[3]);
        }
        clipMatrixDirty=false;
    }
    if(textureMatrixDirty) {
        const auto& m=matrices[MATRIX_TEXTURE];
        C3D_FVUnifSet(GPU_VERTEX_SHADER,textureUniform,m.m[0][0],m.m[1][0],m.m[2][0],m.m[3][0]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,textureUniform+1,-m.m[0][1],-m.m[1][1],-m.m[2][1],1-m.m[3][1]);
        textureMatrixDirty=false;
    }
    // Packed RGBA bytes reach the shader directly instead of expanding four
    // floats on ARM for every vertex. Preserve the engine's RGB channel order.
    const u32 rgba=(factor&0xff00ff00u)|((factor&255)<<16)|((factor>>16)&255);
    Vertex* out=vertices+vertexCursor;
    if(!direct)for(int n=0;n<count;n++) {
        const int i=first+n;
        std::memcpy(&out[n].position.x,static_cast<u8*>(pointers[0])+strides[0]*i,3*sizeof(float));
        if(attributes[0] && pointers[1])
            std::memcpy(&out[n].textureUV.x,static_cast<u8*>(pointers[1])+strides[1]*i,2*sizeof(float));
        else {out[n].textureUV.x=out[n].textureUV.y=0;}
        if(vertexColors)
            std::memcpy(&out[n].diffuse,static_cast<u8*>(pointers[2])+strides[2]*i,sizeof(u32));
        else out[n].diffuse=rgba;
    }
    ApplyFog();
    // These transforms happen after homogeneous clipping. Preserve GL's
    // 0=near/1=far stored depth so its clear value and LEQUAL test still apply.
    if(!applied.valid || std::memcmp(applied.viewport,viewport,sizeof(viewport))!=0) {
        C3D_SetViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        // SetViewport disables scissoring, so restore it with the viewport.
        C3D_SetScissor(GPU_SCISSOR_NORMAL,viewport[0],viewport[1],
                      viewport[0]+viewport[2],viewport[1]+viewport[3]);
        std::memcpy(applied.viewport,viewport,sizeof(viewport));
    }
    if(!applied.valid || applied.depthNear!=depthNear || applied.depthFar!=depthFar)
        C3D_DepthMap(true,depthFar-depthNear,depthFar);
    if(!applied.valid || applied.menu!=drawingMenuLayer || applied.depthTest!=depthTest ||
       applied.depthFunc!=depthFunc || applied.depthMask!=depthMask)
        C3D_DepthTest(drawingMenuLayer?false:depthTest,depthFunc==DEPTH_FUNC_ALWAYS?GPU_ALWAYS:GPU_LEQUAL,
                      !drawingMenuLayer && depthMask?GPU_WRITE_ALL:GPU_WRITE_COLOR);
    if(!applied.valid || applied.blend!=blend)
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,
                      blend==BLEND_ONE?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ONE_MINUS_SRC_ALPHA);
    const bool textured=attributes[0] && bound && bound->ready;
    if(!applied.valid || applied.textured!=textured || applied.colorOps[0]!=colorOps[0] ||
       applied.colorOps[1]!=colorOps[1]) {
        auto* env=C3D_GetTexEnv(0);
        C3D_TexEnvSrc(env,C3D_Both,textured?GPU_TEXTURE0:GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env,C3D_RGB,!textured || colorOps[0]==COLOR_OP_REPLACE?GPU_REPLACE:colorOps[0]==COLOR_OP_ADD?GPU_ADD:GPU_MODULATE);
        C3D_TexEnvFunc(env,C3D_Alpha,!textured || colorOps[1]==COLOR_OP_REPLACE?GPU_REPLACE:GPU_MODULATE);
    }
    C3D_Tex* texture=textured?&bound->gpu:nullptr;
    if(!applied.valid || applied.texture!=texture)C3D_TexBind(0,texture);
    applied.depthNear=depthNear;applied.depthFar=depthFar;applied.menu=drawingMenuLayer;
    applied.depthTest=depthTest;applied.depthFunc=depthFunc;applied.depthMask=depthMask;
    applied.blend=blend;applied.textured=textured;applied.texture=texture;
    applied.colorOps[0]=colorOps[0];applied.colorOps[1]=colorOps[1];applied.valid=true;
    C3D_DrawArrays(type==PRIM_TRIANGLES?GPU_TRIANGLES:GPU_TRIANGLE_STRIP,
                  direct?u32(offset/sizeof(Vertex)):vertexCursor,count);
    if(!direct)vertexCursor+=count;
}
void CtrRenderer::FlushVertices() {
    if(vertexCursor) GSPGPU_FlushDataCache(vertices,sizeof(Vertex)*vertexCursor);
}
void CtrRenderer::EndSegment() {
    // Use citro3d's complete cache flush, including command lists split by
    // clears and citro2d's private presentation buffers.
    C3D_FrameEnd(0);frameOpen=false;
}
void CtrRenderer::SplitSegment() {
    if(queuedCommands>=16)FinishTextureReads();
    C3D_FrameSplit(0);++queuedCommands;
}
void CtrRenderer::SwapBuffers() {
    if(!frameOpen || !sceneComplete) return;
    // C2D_TargetClear splits and may start the pending command list before
    // FrameEnd performs its normal linear-heap flush. Make our external scene
    // vertex buffer visible first so the copy never consumes stale geometry.
    FlushVertices();
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_DepthMap(true,-1,0);
    C2D_Prepare();NativeBlend();
    C2D_TargetClear(top,0xff000000);C2D_TargetClear(bottom,0xff000000);
    CtrPresentation::DrawBackground(top,bottom,gameplayLayout);
    if(gameplayLayout) {
        // Original field (32,16)-(416,464), at uniform 5/6 scale.
        // Alignment changes placement only; size and aspect ratio stay fixed.
        const float offset=CtrLayout::Offset(),scale=CtrLayout::Scale();
        CtrDecorations::DrawPortrait(top);
        Tex3DS_SubTexture sub{384,448,32.0f/1024,464.0f/512,416.0f/1024,16.0f/512};
        C2D_Image image{&scene,&sub};
        C2D_SceneBegin(top);C2D_DrawImageAt(image,CtrLayout::Left(),offset,0,nullptr,scale,scale);
        if(!CtrLayout::Single()) {C2D_SceneBegin(bottom);C2D_DrawImageAt(image,CtrLayout::Left(false),offset-240,0,nullptr,scale,scale);}
    } else {
        Tex3DS_SubTexture sub{640,480,0,480.0f/512,640.0f/1024,0};
        C2D_Image image{&scene,&sub};
        // The transparent menu target already contains RGB multiplied by alpha.
        // Compose it once; multiplying its alpha again darkens every animation.
        C2D_Flush();NativeBlend(true);
        C2D_SceneBegin(top);C2D_DrawImageAt(image,40,0,0,nullptr,0.5f,0.5f);
        C2D_Flush();NativeBlend();
    }
    if(gameplayLayout) {
        CtrDecorations::DrawHitbox(top,bottom);
        CtrHud::Draw(scene,top,bottom);
        CtrPause::Draw(top,bottom);
    } else {
        CtrSelection::DrawBottomPortraits(bottom);
        C2D_Flush();NativeBlend(true);menuLayer.Draw(bottom);C2D_Flush();NativeBlend();
        CtrPresentation::DrawMenu(bottom);
        CtrSelection::DrawTop(top);
        CtrHud::DrawMenuFps(top);
    }
    CtrPresentation::DrawOverlays(top,bottom);
    C2D_Flush();
    EndSegment();
    if(gameplayLayout)CtrPresentation::ReleaseMenuAssets();
    BeginFrame();
}



void CtrRenderer::BeginMenuLayer(bool clear) {
    if(!frameOpen)BeginFrame();
    if(!frameOpen)return;
    FlushVertices();SplitSegment();++queuedCommands;
    std::copy(viewport,viewport+4,savedViewport);
    menuLayer.Begin(clear);drawingMenuLayer=true;
    SetViewport(0,0,640,480);ApplyState();
}
void CtrRenderer::EndMenuLayer() {
    if(!frameOpen)return;
    drawingMenuLayer=false;std::copy(savedViewport,savedViewport+4,viewport);
    C3D_FrameDrawOn(sceneTarget);ApplyState();
}
void CtrRenderer::ShowLoadingScreen() {
    // Loading retains both completed displays until the new scene is ready.
    if(sceneComplete) SwapBuffers();
}
void CtrRenderer::SuspendForApplet() {
    if(exiting || suspended || aptShouldClose())return;
    FlushVertices();
    if(frameOpen)EndSegment();
    C3D_FrameSync();
}
void CtrRenderer::ResumeFromApplet() {BeginFrame();}

C3D_Tex* CtrRenderer::TextureImage(GfxTextureHandle handle) {
    if(handle.id>=textures.size() || !textures[handle.id] || !textures[handle.id]->ready)return nullptr;
    return &textures[handle.id]->gpu;
}

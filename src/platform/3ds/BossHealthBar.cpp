#include "BossHealthBar.hpp"
#include "Settings.hpp"
#include "CtrRenderer.hpp"
#include "AnmManager.hpp"
#include "EnemyManager.hpp"
#include "Gui.hpp"
#include <algorithm>
#include <cmath>

namespace {
// TH20 front.anm scripts 374-377: 38 segments, 112px main radius,
// 8px width, borders at 108/116, 10-frame entrance and 20-frame exit.
// Its playfield is twice EoSD's size; halve lengths, retain fade durations.
constexpr int Segments=38;
constexpr float Tau=6.28318530718f;
ZunVec2 unit[Segments+1];
GfxTextureHandle texture{};
const Enemy* actor=nullptr;
ZunVec2 center{};
float opacity=0,health=0,marker=-1;

ZunVec2 At(float fraction) {
 const float scaled=std::clamp(fraction,0.f,1.f)*Segments;
 const int index=std::min(Segments,int(scaled));
 if(index==Segments || scaled==index)return unit[index];
 const float angle=-Tau*fraction-Tau*.25f;
 return {std::cos(angle),std::sin(angle)};
}
void Quad(const ZunVec2* points,float x0,float y0,float x1,float y1) {
 VertexTex1Xyzrhw vertices[4]{};
 for(int i=0;i<4;i++) {
  vertices[i].position=ZunVec4(center.x+points[i].x,center.y+points[i].y,.495f,1);
  vertices[i].textureUV=ZunVec2((i&1?x1:x0)/32.f,(i>=2?y1:y0)/32.f);
 }
 g_AnmManager->AddSpriteToDrawBuffer(vertices);
}
void Arc(float radius,float width,float fraction,float x0,float x1) {
 const float end=std::clamp(fraction,0.f,1.f)*Segments;
 for(int segment=0;segment<int(std::ceil(end));segment++) {
  const auto& a=unit[segment];
  const auto b=end>=segment+1?unit[segment+1]:At(fraction);
  const float inner=radius-width*.5f,outer=radius+width*.5f;
  ZunVec2 points[]={{a.x*inner,a.y*inner},{a.x*outer,a.y*outer},
                   {b.x*inner,b.y*inner},{b.x*outer,b.y*outer}};
  Quad(points,x0,.5f,x1,31.5f);
 }
}
void PhaseMarker() {
 if(marker<=0 || marker>=1)return;
 const auto radial=At(marker);
 const ZunVec2 tangent{-radial.y,radial.x};
 ZunVec2 points[4];
 for(int i=0;i<4;i++) {
  const float r=i>=2?59.f:53.f,t=i&1?1.5f:-1.5f;
  points[i]={radial.x*r+tangent.x*t,radial.y*r+tangent.y*t};
 }
 Quad(points,20.5f,.5f,25.5f,11.5f);
}
}
bool CtrBossHealthBar::Initialize(CtrRenderer& renderer) {
 if(texture)return true;
 u32 width=0,height=0;
 texture=renderer.LoadPreparedTexture("boss-lifebar",width,height);
 for(int i=0;i<=Segments;i++) {
  const float angle=-Tau*i/Segments-Tau*.25f;
  unit[i]={std::cos(angle),std::sin(angle)};
 }
 return true; // TH20 artwork is optional; the original bar remains available.
}
bool CtrBossHealthBar::Available() {return texture!=0;}
void CtrBossHealthBar::Shutdown(CtrRenderer& renderer) {if(texture)renderer.DeleteTexture(texture);texture={};Reset();}
void CtrBossHealthBar::Reset() {actor=nullptr;center={};opacity=health=0;marker=-1;}
void CtrBossHealthBar::Tick() {
 const Enemy* next=nullptr;
 if(g_Gui.bossPresent && !g_Gui.HasCurrentMsgIdx() && !g_Gui.impl->finishedStage)
  for(const auto* boss:g_EnemyManager.bosses)
   if(boss && boss->flags.active && boss->flags.isBoss && boss->maxLife>0){next=boss;break;}
 if(next) {
  if(next!=actor){health=0;actor=next;}
  center={32+next->position.x,16+next->position.y};
  const float target=std::clamp(next->LifePercent(),0.f,1.f);
  health=std::clamp(target,health-.04f,health+.1f);
  marker=next->lifeCallbackThreshold>0?float(next->lifeCallbackThreshold)/next->maxLife:-1;
 }
 opacity=std::clamp(opacity+(next?.1f:-.05f),0.f,1.f);
 if(!next && opacity==0)actor=nullptr;
}
void CtrBossHealthBar::Draw() {
 if(!texture || opacity<=0 || !CtrSettings::Enabled(CtrSettings::Toggle::BossBar) ||
    g_Gui.HasCurrentMsgIdx() || g_Gui.impl->finishedStage)return;
 auto& manager=*g_AnmManager;
 manager.SetProjectionMode(PROJECTION_MODE_ORTHOGRAPHIC);
 manager.SetCurrentSprite(nullptr);manager.SetCurrentTexture(texture);
 manager.SetVertexAttributes(VERTEX_ATTR_TEX_COORD);
 AnmVm state{};state.flags.zWriteDisable=1;
 state.color=(u32(opacity*255)<<24)|0xffffff;
 manager.SetRenderStateForVm(&state);
 manager.SetDepthFunc(DEPTH_FUNC_ALWAYS);
 manager.SetColorOp(COMPONENT_RGB,COLOR_OP_MODULATE);
 manager.SetColorOp(COMPONENT_ALPHA,COLOR_OP_MODULATE);
 // The opaque white stripe already renders after bullets. A dark backing
 // under its translucent edges keeps its contour visible over white bullets.
 state.color&=0xff000000;
 manager.SetRenderStateForVm(&state);
 Arc(56,4,health,3.5f,6.5f);
 state.color|=0xffffff;
 manager.SetRenderStateForVm(&state);
 Arc(54,1,1,12.5f,15.5f);
 Arc(58,1,1,12.5f,15.5f);
 Arc(56,4,health,.5f,7.5f);
 PhaseMarker();
 manager.FlushVertexBuffer();
 manager.SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
}

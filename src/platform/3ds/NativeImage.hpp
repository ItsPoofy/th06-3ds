#pragma once
#include <citro2d.h>
namespace CtrNative {
inline void FinishImage(bool drawn,bool additive=false,bool subtractAlpha=false) {
    if(drawn) {
        auto* env=C3D_GetTexEnv(0);C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env,C3D_RGB,additive?GPU_ADD:GPU_MODULATE);
        C3D_TexEnvFunc(env,C3D_Alpha,subtractAlpha?GPU_SUBTRACT:GPU_MODULATE);
        for(int i=1;i<5;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C2D_Flush();
    }
    C2D_SetTintMode(C2D_TintSolid);
}
// Ordinary opacity changes stay in citro2d's batch. Colored, additive and
// subtractive draws need the explicit native texture-environment expression.
inline void DrawImage(C2D_Image image,float x,float y,unsigned color=0xffffffff,float sx=1,float sy=1,bool additive=false) {
    // Opaque, untinted atlas images can share citro2d's ordinary batch. Most
    // HUD labels, digits and stars take this path instead of flushing per glyph.
    if(color==0xffffffff && !additive) {C2D_DrawImageAt(image,x,y,0,nullptr,sx,sy);return;}
    // Solid tint with zero RGB blend preserves texture color while multiplying
    // alpha. Translucent portraits, namecards and HUD fades share one batch.
    if((color&0xffffff)==0xffffff && !additive) {
        C2D_SetTintMode(C2D_TintSolid);C2D_ImageTint tint;C2D_PlainImageTint(&tint,color,0);
        C2D_DrawImageAt(image,x,y,0,&tint,sx,sy);return;
    }
    C2D_Flush();C2D_SetTintMode(C2D_TintMult);
    C2D_ImageTint tint;C2D_PlainImageTint(&tint,color,1);
    FinishImage(C2D_DrawImageAt(image,x,y,0,&tint,sx,sy),additive);
}
inline void DrawTextGradient(C2D_Image image,float x,float y,unsigned color,float faceTop,float faceBottom,float sx=1,float sy=1) {
    // Share the normal glyph's antialiasing and opacity. Only its face tint
    // changes; three adjacent quads retain a pure-white upper highlight.
    const auto& source=*image.subtex;
    const float start=faceTop+(faceBottom-faceTop)*.18f;
    const float cuts[]={0,start,faceBottom,float(source.height)};
    const unsigned white=(color&0xff000000)|0xffffff;
    C2D_Flush();C2D_SetTintMode(C2D_TintMult);
    bool drawn=false;
    for(int i=0;i<3;i++) {
        Tex3DS_SubTexture sub=source;
        sub.height=u16(cuts[i+1]-cuts[i]);
        if(!sub.height)continue;
        sub.top=source.top+(source.bottom-source.top)*cuts[i]/source.height;
        sub.bottom=source.top+(source.bottom-source.top)*cuts[i+1]/source.height;
        C2D_ImageTint tint;C2D_PlainImageTint(&tint,i==2?color:white,1);
        if(i==1)C2D_BottomImageTint(&tint,color,1);
        drawn=C2D_DrawImageAt({image.tex,&sub},x,y+cuts[i]*sy,0,&tint,sx,sy*(cuts[i+1]-cuts[i])/sub.height) || drawn;
    }
    FinishImage(drawn);
}
inline void DrawImageRotated(C2D_Image image,float centerX,float centerY,float angle,unsigned color,
                             float sx=1,float sy=1,bool subtractAlpha=false,bool additive=false) {
    if(additive) {
        C2D_Flush();C2D_SetTintMode(C2D_TintSolid);
        C2D_ImageTint tint;C2D_PlainImageTint(&tint,color,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE,GPU_ONE,GPU_ONE);
        C2D_DrawImageAtRotated(image,centerX,centerY,0,angle,&tint,sx,sy);
        C2D_Flush();
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ONE_MINUS_SRC_ALPHA);
        return;
    }
    if(color==0xffffffff && !subtractAlpha) {C2D_DrawImageAtRotated(image,centerX,centerY,0,angle,nullptr,sx,sy);return;}
    if((color&0xffffff)==0xffffff && !subtractAlpha) {
        C2D_SetTintMode(C2D_TintSolid);C2D_ImageTint tint;C2D_PlainImageTint(&tint,color,0);
        C2D_DrawImageAtRotated(image,centerX,centerY,0,angle,&tint,sx,sy);return;
    }
    C2D_Flush();C2D_SetTintMode(C2D_TintMult);
    C2D_ImageTint tint;C2D_PlainImageTint(&tint,color,1);
    FinishImage(C2D_DrawImageAtRotated(image,centerX,centerY,0,angle,&tint,sx,sy),false,subtractAlpha);
}
}

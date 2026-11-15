#pragma once

#include "AnmManager.hpp"
#include "ZunColor.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

// #include <d3d8.h>

// struct FormatInfo
//{
//     D3DFORMAT format;
//     i32 bitCount;
//     u32 alphaMask;
//     u32 redMask;
//     u32 greenMask;
//     u32 blueMask;
// };
struct TextHelper
{
    enum class FontRole { Standard, SpellTitle, MusicTitle };
    static ZunResult CreateTextBuffer();
    static void ReleaseTextBuffer();
    static void StageCaption(AnmVm* vm,const char* text,ZunColor color,ZunColor shadow);
    static void Wrapped(AnmVm* vm,const char* text,ZunColor color,ZunColor shadow,int width,bool japaneseSingleLine=false,bool centered=false,bool rightAligned=false,bool outlined=false,FontRole role=FontRole::Standard);
    static void Precache(const char* text,i32 fontHeight);
    static void RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                    i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                    TextureData *outTexture, bool balanceSeparator=false,int* renderedHeight=nullptr,int wrapWidth=230,bool forceWrap=false,bool centered=true,bool rightAligned=false,bool outlined=false,FontRole role=FontRole::Standard);

    TextHelper();
    ~TextHelper();
    //    bool AllocateBufferWithFallback(i32 width, i32 height, D3DFORMAT format);
    //    bool TryAllocateBuffer(i32 width, i32 height, D3DFORMAT format);
    //    FormatInfo *GetFormatInfo(D3DFORMAT format);
    //    bool ReleaseBuffer() const;
    static float Measure(const char* text,int fontHeight);
    static int PrefixWidths(const char* text,int fontHeight,float* widths,int capacity);
    static bool InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight);
    //    bool CopyTextToSurface(IDirect3DSurface8 *outSurface);

    //    D3DFORMAT format;
    i32 width;
    i32 height;
    u32 imageSizeInBytes;
    i32 imageWidthInBytes;
    //    HDC hdc;
    //    HGDIOBJ gdiObj;
    //    HGDIOBJ gdiObj2;
    u8 *buffer;
};

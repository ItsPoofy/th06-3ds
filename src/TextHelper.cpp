#include <string>
#include "TextHelper.hpp"
#include "StageCaptionSurface.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

#include "thirdparty/sjis_converter.h"

#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <array>
#include <map>
#include <new>
#include "FileSystem.hpp"
#ifdef __3DS__
#include "platform/3ds/CtrRenderer.hpp"
#include "platform/3ds/ContentAssets.hpp"
#include "platform/3ds/Settings.hpp"
#endif

static TTF_Font *g_Font;
static std::array<std::vector<u8>,3> fontData;
static std::map<std::pair<int,int>,TTF_Font*> fonts;
static TTF_Font* FontAtSize(int size,TextHelper::FontRole role=TextHelper::FontRole::Standard) {
#ifdef __3DS__
    // This is the original MS Gothic text path. Bitmap and menu typefaces
    // are separate assets and retain their own fonts.
    const bool english=CtrSettings::English() && !fontData[1].empty();
#else
    const bool english=false;
#endif
    const bool scaled=role==TextHelper::FontRole::SpellTitle || role==TextHelper::FontRole::MusicTitle;
    const int face=english?(scaled && !fontData[2].empty()?2:1):0;
    const auto key=std::make_pair(face,size);
    if(auto found=fonts.find(key);found!=fonts.end()) return found->second;
    const auto& data=fontData[face];
    // Match visible MS Gothic letter height; Biolinum has smaller ink at equal point sizes.
    const int rasterSize=english?(size*22+7)/15:size;
    auto* font=TTF_OpenFontRW(SDL_RWFromConstMem(data.data(),data.size()),1,rasterSize);
    if(font) fonts[key]=font;
    return font;
}
static void ReadFont(const char* path,std::vector<u8>& data) {
    if(FILE* file=FileSystem::FopenUTF8(path,"rb")) {
        fseek(file,0,SEEK_END);data.resize(ftell(file));rewind(file);
        if(fread(data.data(),1,data.size(),file)!=data.size())data.clear();
        fclose(file);
    }
}

TextHelper::TextHelper()
{
    //    this->format = (D3DFORMAT)-1;
    //    this->width = 0;
    //    this->height = 0;
    //    this->hdc = 0;
    //    this->gdiObj2 = 0;
    //    this->gdiObj = 0;
    //    this->buffer = NULL;
}

TextHelper::~TextHelper()
{
    TTF_Quit();
}

#define TEXT_BUFFER_HEIGHT 64

// Extended to initialize all globals for text helper
ZunResult TextHelper::CreateTextBuffer()
{
    TTF_Init();

    ReadFont(TH_PRIMARY_FONT_FILENAME,fontData[0]);
    if(fontData[0].empty())ReadFont(TH_FALLBACK_FONT_FILENAME,fontData[0]);
#ifdef __3DS__
    ReadFont(TH_ENGLISH_FONT_FILENAME,fontData[1]);
    ReadFont(TH_ENGLISH_SPELL_FONT_FILENAME,fontData[2]);
#endif
    g_Font=fontData[0].empty()?nullptr:FontAtSize(10);
    if(!g_Font)
    {
        std::printf("%s\n", TTF_GetError());

        g_GameErrorContext.Fatal(TH_ERR_FONTS_NOT_FOUND);
        return ZUN_ERROR;
    }

    g_TextBufferSurface =
        SDL_CreateRGBSurfaceWithFormat(0, GAME_WINDOW_WIDTH, TEXT_BUFFER_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);

    if(!g_TextBufferSurface)return ZUN_ERROR;
    SDL_SetSurfaceBlendMode(g_TextBufferSurface, SDL_BLENDMODE_NONE);

    return ZUN_SUCCESS;
}

bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight)
{
    if(!g_TextBufferSurface || spriteWidth<=0 || fontHeight<=0)return false;
    SDL_LockSurface(g_TextBufferSurface);
    // The shared scratch surface can be wider than this label after rendering
    // a music title. Use its actual pitch, so the gradient never wraps across
    // unrelated rows or changes with the previously rendered label's width.
    for(int row=std::max<i32>(0,y);row<std::min<i32>(y+fontHeight,g_TextBufferSurface->h);row++) {
        auto* pixels=static_cast<u8*>(g_TextBufferSurface->pixels)+row*g_TextBufferSurface->pitch;
        const unsigned rg=256-(row-y)*128/fontHeight,b=256-(row-y)*64/fontHeight;
        for(int col=std::max<i32>(0,x);col<std::min<i32>(x+spriteWidth,g_TextBufferSurface->w);col++) {
            auto* pixel=pixels+col*4;
            if(pixel[3]) {pixel[0]=pixel[0]*rg>>8;pixel[1]=pixel[1]*rg>>8;pixel[2]=pixel[2]*b>>8;}
        }
    }
    SDL_UnlockSurface(g_TextBufferSurface);

    return true;
}

// Text strings in asset files are encoded using Shift_JIS. This allows RenderTextToTexture to handle both UTF-8 and
// Shift_JIS. This also does not check for overlong encoding, but that shouldn't matter
bool isUTF8Encoded(const char *string)
{
#define UTF8_1BYTE_MASK 0x80
#define UTF8_2BYTE_MASK 0xE0
#define UTF8_3BYTE_MASK 0xF0
#define UTF8_4BYTE_MASK 0xF8

#define UTF8_2NDBYTE_MASK 0xC0

// 0xxx xxxx
#define UTF8_1BYTE_PREFIX 0x00
// 110x xxxx
#define UTF8_2BYTE_PREFIX 0xC0
// 1110 xxxx
#define UTF8_3BYTE_PREFIX 0xE0
// 1111 0xxx
#define UTF8_4BYTE_PREFIX 0xF0

// 10xx xxxx
#define UTF8_2NDBYTE_PREFIX 0x80

    bool isMultiByteParse = false;
    int codepointLen = 0;

    while (*string != '\0')
    {
        unsigned char c = *(unsigned char *)string;

        if (!isMultiByteParse)
        {
            if ((c & UTF8_1BYTE_MASK) != UTF8_1BYTE_PREFIX)
            {
                isMultiByteParse = true;

                if ((c & UTF8_2BYTE_MASK) == UTF8_2BYTE_PREFIX)
                    codepointLen = 1;
                else if ((c & UTF8_3BYTE_MASK) == UTF8_3BYTE_PREFIX)
                    codepointLen = 2;
                else if ((c & UTF8_4BYTE_MASK) == UTF8_4BYTE_PREFIX)
                    codepointLen = 3;
                else
                    return false;
            }
        }
        else
        {
            if ((c & UTF8_2NDBYTE_MASK) != UTF8_2NDBYTE_PREFIX)
                return false;

            if (--codepointLen == 0)
                isMultiByteParse = false;
        }

        string++;
    }

    return true;

#undef UTF8_1BYTE_MASK
#undef UTF8_2BYTE_MASK
#undef UTF8_3BYTE_MASK
#undef UTF8_4BYTE_MASK

#undef UTF8_2NDBYTE_MASK

#undef UTF8_1BYTE_PREFIX
#undef UTF8_2BYTE_PREFIX
#undef UTF8_3BYTE_PREFIX
#undef UTF8_4BYTE_PREFIX

#undef UTF8_2NDBYTE_PREFIX
}

void SurfaceOverwriteBlend(SDL_Surface *srcSurface, SDL_Surface *dstSurface, i32 x)
{
    // Source surface is A8R8G8B8
    // Dest surface is RGBA32
    // We want to overwrite dest unless source has alpha 0

    SDL_LockSurface(srcSurface);
    SDL_LockSurface(dstSurface);

    u32 *srcData = (u32 *)srcSurface->pixels;
    u8 *dstData = (u8 *)dstSurface->pixels;

    for (int i = 0; i < std::min(srcSurface->h,dstSurface->h); i++)
    {
        for (int j = std::max<i32>(0,-x); j < std::min<i32>(srcSurface->w,dstSurface->w-x); j++)
        {
            if ((srcData[j] & 0xFF00'0000) != 0)
            {
                dstData[i * dstSurface->pitch + (x + j) * 4] = (srcData[j] >> 16) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 1] = (srcData[j] >> 8) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 2] = srcData[j] & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 3] = (srcData[j] >> 24) & 0xFF;
            }
        }

        srcData += srcSurface->pitch / 4;
    }

    SDL_UnlockSurface(dstSurface);
    SDL_UnlockSurface(srcSurface);
}

void TextHelper::RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                     i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                     TextureData *outTexture, bool balanceSeparator,int* renderedHeight,int wrapWidth,bool forceWrap,bool centered,bool rightAligned,bool outlined,FontRole role)
{
    char convertedText[1024];
    SDL_Rect finalCopyDst;
    SDL_Rect finalCopySrc;
    SDL_Rect shadowRect;
    SDL_Rect textRect;

    if (!isUTF8Encoded(string))
    {
        char *utf8 = sjis2utf8(string);
        if(!utf8)return;
        std::snprintf(convertedText,sizeof(convertedText),"%s",utf8);
        free(utf8);
    }
    else
    {
        std::snprintf(convertedText,sizeof(convertedText),"%s",string);
    }

#ifdef __3DS__
    const std::string translated=CtrContent::Translate(convertedText);
    std::snprintf(convertedText,sizeof(convertedText),"%s",translated.c_str());
#endif
    g_Font=FontAtSize(fontHeight*2,role);
    if(!g_Font) return;
#ifdef __3DS__
    const bool english=CtrSettings::English() && !fontData[1].empty();
    const bool twoRows=balanceSeparator && (CtrSettings::English() || forceWrap) &&
        !(english && role==FontRole::MusicTitle);
#else
    const bool english=false;
    const bool twoRows=balanceSeparator;
#endif
    SDL_Color sdlTextColor{Uint8(textColor&0xff),Uint8((textColor>>8)&0xff),Uint8((textColor>>16)&0xff),255};
    // Fit at the largest caption draw scale, preserving the same font in both layouts.
    const int captionWidth=2*wrapWidth-(outlined?8:0);
    SDL_Surface* regularText=balanceSeparator?StageCaptionSurface::Render(g_Font,convertedText,sdlTextColor,twoRows,captionWidth,centered,rightAligned):
        (english?StageCaptionSurface::RenderLine(g_Font,convertedText,sdlTextColor,(fontHeight*8+2)/3):
            TTF_RenderUTF8_Blended(g_Font,convertedText,sdlTextColor));
    if(outlined && regularText) {
        auto* outline=StageCaptionSurface::WhiteOutlined(regularText,4);
        if(outline) {SDL_FreeSurface(regularText);regularText=outline;}
    }
    const int captionHeight=regularText?regularText->h:32;
    const int copyHeight=balanceSeparator || english?(captionHeight+1)/2:16;
    if(renderedHeight)*renderedHeight=copyHeight;
    const int scratchWidth=std::max<i32>(2*spriteWidth,GAME_WINDOW_WIDTH);
    const int scratchHeight=std::max<i32>(balanceSeparator || english?captionHeight+6:2*fontHeight+6,TEXT_BUFFER_HEIGHT);
    if(!g_TextBufferSurface || g_TextBufferSurface->w<scratchWidth || g_TextBufferSurface->h<scratchHeight) {
        SDL_FreeSurface(g_TextBufferSurface);
        g_TextBufferSurface=SDL_CreateRGBSurfaceWithFormat(0,scratchWidth,scratchHeight,32,SDL_PIXELFORMAT_RGBA32);
    }
    if(!g_TextBufferSurface) {SDL_FreeSurface(regularText);return;}

    finalCopySrc.x = 0;
    finalCopySrc.y = 0;
    finalCopySrc.w = spriteWidth * 2 - 2;
    finalCopySrc.h = fontHeight * 2 - 2;
    if(balanceSeparator || english) {finalCopySrc.w=spriteWidth*2;finalCopySrc.h=captionHeight;}

    SDL_FillRect(g_TextBufferSurface, nullptr, 0);

    if (!outlined && shadowColor != COLOR_WHITE)
    {
        SDL_Surface *shadowText;

        // Render shadow.
        SDL_Color sdlShadowColor;
        sdlShadowColor.a = 0xFF;
        sdlShadowColor.b = (shadowColor >> 16) & 0xFF;
        sdlShadowColor.g = (shadowColor >> 8) & 0xFF;
        sdlShadowColor.r = shadowColor & 0xFF;

        shadowText = balanceSeparator?StageCaptionSurface::Render(g_Font,convertedText,sdlShadowColor,twoRows,captionWidth,centered,rightAligned):(english?StageCaptionSurface::RenderLine(g_Font,convertedText,sdlShadowColor,(fontHeight*8+2)/3):TTF_RenderUTF8_Blended(g_Font, convertedText, sdlShadowColor));

        if (shadowText != NULL)
        {
            shadowRect.x = xPos * 2 + 3;
            shadowRect.y = 2;
            shadowRect.w = shadowText->w;
            shadowRect.h = shadowText->h;

            SDL_SetSurfaceBlendMode(shadowText, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(shadowText, NULL, g_TextBufferSurface, &shadowRect);

            SDL_FreeSurface(shadowText);
        }
    }

    if (regularText != NULL)
    {
        textRect.x = xPos * 2;
        textRect.y = 0;
        textRect.w = regularText->w;
        textRect.h = regularText->h;

        SurfaceOverwriteBlend(regularText, g_TextBufferSurface, xPos * 2);

        SDL_FreeSurface(regularText);
    }

    // Once we get an API abstraction layer for surface operations, this needs to change
    //   We really shouldn't be clobbering the texture format
    if (!outTexture->textureData || outTexture->format != TEX_FMT_A8R8G8B8)
    {
        delete[] outTexture->textureData;
        outTexture->textureData = new(std::nothrow) u8[outTexture->width * outTexture->height * 4];
        if(!outTexture->textureData)return;
        memset(outTexture->textureData, 0, outTexture->width * outTexture->height * 4);
    }

    outTexture->format = TEX_FMT_A8R8G8B8;
    SDL_Surface *textureSurface = SDL_CreateRGBSurfaceWithFormatFrom(
        outTexture->textureData, outTexture->width, outTexture->height, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGBA32),
        outTexture->width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32), SDL_PIXELFORMAT_RGBA32);

    if(!textureSurface)return;
    if(twoRows) {
        for(int row=0;row<captionHeight;row+=english?48:StageCaptionSurface::RowStep)
            InvertAlpha(0,row,spriteWidth*2,english?44:StageCaptionSurface::RowHeight+4);
    } else InvertAlpha(0, 0, spriteWidth * 2, captionHeight + 6);

    finalCopyDst.x = 0;
    finalCopyDst.y = yPos;
    finalCopyDst.w = spriteWidth;
    finalCopyDst.h = copyHeight;

    if (SDL_SoftStretchLinear(g_TextBufferSurface, &finalCopySrc, textureSurface, &finalCopyDst) < 0)
    {
        SDL_Log("SDL_BlitScaled failed! Error: %s", SDL_GetError());
    }

    g_AnmManager->SetCurrentTexture(outTexture->handle);

#ifdef __3DS__
    static_cast<CtrRenderer*>(g_GfxBackend)->UpdateTextRows(finalCopyDst.y,finalCopyDst.w,finalCopyDst.h,
        outTexture->textureData+finalCopyDst.y*outTexture->width*4);
#else
    g_GfxBackend->SetTextureImage(outTexture->width,outTexture->height,PIXEL_RGBA,PIXEL_UNSIGNED_BYTE,outTexture->textureData);
#endif

    SDL_FreeSurface(textureSurface);

    return;
}

// Extended to free all globals for text helper
void TextHelper::ReleaseTextBuffer()
{
    for(auto [size,font]:fonts) TTF_CloseFont(font);
    fonts.clear();for(auto& data:fontData)data.clear();g_Font=nullptr;

    if (g_TextBufferSurface != NULL)
    {
        SDL_FreeSurface(g_TextBufferSurface);
        g_TextBufferSurface = NULL;
    }

    return;
}
void TextHelper::Precache(const char* text,i32 fontHeight) {
    char* converted=nullptr;
    if(!isUTF8Encoded(text)) {converted=sjis2utf8(text);text=converted;}
#ifdef __3DS__
    if(text)text=CtrContent::Translate(text);
#endif
    if(auto* font=FontAtSize(fontHeight*2)) {
        auto* rendered=TTF_RenderUTF8_Blended(font,text,SDL_Color{255,255,255,255});
        SDL_FreeSurface(rendered);
    }
    free(converted);
}

float TextHelper::Measure(const char* text,int fontHeight) {
    char* converted=nullptr;
    if(!isUTF8Encoded(text)) {converted=sjis2utf8(text);if(!converted)return 0;text=converted;}
#ifdef __3DS__
    text=CtrContent::Translate(text);
#endif
    int width=0,height=0;
    if(auto* font=FontAtSize(std::max(1,fontHeight)*2))TTF_SizeUTF8(font,text,&width,&height);
    free(converted);return width*.5f+2;
}
int TextHelper::PrefixWidths(const char* text,int fontHeight,float* widths,int capacity) {
    char* converted=nullptr;
    if(!isUTF8Encoded(text)) {converted=sjis2utf8(text);if(!converted)return 0;text=converted;}
#ifdef __3DS__
    text=CtrContent::Translate(text);
#endif
    auto* font=FontAtSize(std::max(1,fontHeight)*2);
    int count=0,total=0;Uint32 previous=0;
    if(font)for(const auto* c=reinterpret_cast<const unsigned char*>(text);*c && count+1<capacity;) {
        Uint32 point=*c++;int extra=0;
        if(point>=0xf0){point&=7;extra=3;}else if(point>=0xe0){point&=15;extra=2;}else if(point>=0xc0){point&=31;extra=1;}
        bool valid=true;
        while(extra--){if((*c&0xc0)!=0x80){valid=false;break;}point=(point<<6)|(*c++&63);}
        if(!valid)break;
        int advance=0;
        if(TTF_GlyphMetrics32(font,point,nullptr,nullptr,nullptr,nullptr,&advance)!=0)break;
        if(previous && TTF_GetFontKerning(font))total+=TTF_GetFontKerningSizeGlyphs32(font,previous,point);
        total+=advance;previous=point;widths[++count]=total*.5f+2;
    }
    free(converted);return count;
}

#ifdef __3DS__
void TextHelper::StageCaption(AnmVm* vm,const char* text,ZunColor color,ZunColor shadow) {
    Wrapped(vm,text,color,shadow,230,true,true);
}
void TextHelper::Wrapped(AnmVm* vm,const char* text,ZunColor color,ZunColor shadow,int width,bool japaneseSingleLine,bool centered,bool rightAligned,bool outlined,FontRole role) {
    auto& texture=g_AnmManager->textures[vm->sprite->sourceFileIndex];
    const int row=vm->sprite->startPixelInclusive.y;
    int height=0;
    RenderTextToTexture(0,row,vm->sprite->textureWidth,vm->sprite->textureHeight,
        vm->fontWidth?vm->fontWidth:15,vm->fontHeight?vm->fontHeight:15,color,shadow,text,&texture,true,&height,width,!japaneseSingleLine,centered,rightAligned,outlined,role);
    int left=texture.width,right=0;
    if(texture.textureData)for(int y=row;y<std::min<int>(row+height,texture.height);y++)
        for(unsigned x=0;x<texture.width;x++)if(texture.textureData[(y*texture.width+x)*4+3]) {
            left=std::min(left,int(x));right=std::max(right,int(x)+1);
        }
    vm->ctrTextStart=right>left?left:0;vm->ctrTextWidth=std::max(0,right-left);vm->ctrTextHeight=height;vm->flags.isVisible=true;
}
#endif

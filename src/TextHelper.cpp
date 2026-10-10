#include <string>
#include "TextHelper.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

#include "thirdparty/sjis_converter.h"

#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <map>
#include <new>
#include "FileSystem.hpp"
#ifdef __3DS__
#include "platform/3ds/CtrRenderer.hpp"
#include "platform/3ds/ContentAssets.hpp"
#endif

static TTF_Font *g_Font;
static std::vector<u8> fontData;
static std::map<int,TTF_Font*> fonts;
static TTF_Font* FontAtSize(int size) {
    if(auto found=fonts.find(size);found!=fonts.end()) return found->second;
    auto* font=TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(),fontData.size()),1,size);
    if(font) fonts[size]=font;
    return font;
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

    FILE* file=FileSystem::FopenUTF8(TH_PRIMARY_FONT_FILENAME,"rb");
    if(!file) file=FileSystem::FopenUTF8(TH_FALLBACK_FONT_FILENAME,"rb");
    if(file) {
        fseek(file,0,SEEK_END);fontData.resize(ftell(file));rewind(file);
        if(fread(fontData.data(),1,fontData.size(),file)!=fontData.size()) fontData.clear();
        fclose(file);
    }
    g_Font=fontData.empty()?nullptr:FontAtSize(10);
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

// Stage captions keep their title and subtitle on separate full-size rows.
// Align visible ink, including the wave separator, rather than whitespace.
static SDL_Surface* RenderStageCaption(TTF_Font* font,const char* input,SDL_Color color) {
    std::string text(input);
    for(const char* token:{"\xef\xbd\x9e","\xe3\x80\x9c","\xe2\x88\xbc"})
        for(size_t p=0;(p=text.find(token,p))!=std::string::npos;)text.replace(p,std::strlen(token),"~");
    for(size_t p=0;(p=text.find("\xe3\x80\x80",p))!=std::string::npos;)text.replace(p,3," ");
    const auto separator=text.find('~');
    std::string parts[]={separator==std::string::npos?text:text.substr(0,separator),
                         separator==std::string::npos?"":"~",
                         separator==std::string::npos?"":text.substr(separator+1)};
    SDL_Surface* surfaces[3]{};SDL_Rect bounds[3]{};
    const int gap=TTF_FontHeight(font)/4;
    for(int i=0;i<3;i++) {
        const auto first=parts[i].find_first_not_of(' '),last=parts[i].find_last_not_of(' ');
        parts[i]=first==std::string::npos?"":parts[i].substr(first,last-first+1);
        if(parts[i].empty())continue;
        surfaces[i]=TTF_RenderUTF8_Blended(font,parts[i].c_str(),color);
        if(!surfaces[i])continue;
        auto* surface=surfaces[i];int left=surface->w,top=surface->h,right=0,bottom=0;
        for(int y=0;y<surface->h;y++)for(int x=0;x<surface->w;x++) {
            u32 pixel;std::memcpy(&pixel,static_cast<u8*>(surface->pixels)+y*surface->pitch+x*4,4);
            if(pixel&surface->format->Amask){left=std::min(left,x);right=std::max(right,x+1);top=std::min(top,y);bottom=std::max(bottom,y+1);}
        }
        bounds[i]={left,top,std::max(0,right-left),std::max(0,bottom-top)};
    }
    const int subtitleWidth=bounds[1].w+(surfaces[1] && surfaces[2]?gap:0)+bounds[2].w;
    const int width=std::max(1,std::max(bounds[0].w,subtitleWidth));
    auto* output=SDL_CreateRGBSurfaceWithFormat(0,width,72,32,SDL_PIXELFORMAT_RGBA32);
    if(output) {
        SDL_FillRect(output,nullptr,0);
        int x=(width-subtitleWidth)/2;
        for(int i=0;i<3;i++)if(surfaces[i]) {
            const int row=i==0?(separator==std::string::npos?20:0):40;
            SDL_Rect dst{i==0?(width-bounds[0].w)/2:x,row+(32-bounds[i].h)/2,0,0};
            SDL_SetSurfaceBlendMode(surfaces[i],SDL_BLENDMODE_NONE);
            SDL_BlitSurface(surfaces[i],&bounds[i],output,&dst);
            if(i>0)x+=bounds[i].w+gap;
        }
    }
    for(auto* surface:surfaces)SDL_FreeSurface(surface);return output;
}

void TextHelper::RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                     i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                     TextureData *outTexture, bool balanceSeparator)
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
    g_Font=FontAtSize(fontHeight*2);
    if(!g_Font) return;
    const int scratchWidth=std::max<i32>(2*spriteWidth,GAME_WINDOW_WIDTH);
    const int scratchHeight=std::max<i32>(balanceSeparator?78:2*fontHeight+6,TEXT_BUFFER_HEIGHT);
    if(!g_TextBufferSurface || g_TextBufferSurface->w<scratchWidth || g_TextBufferSurface->h<scratchHeight) {
        SDL_FreeSurface(g_TextBufferSurface);
        g_TextBufferSurface=SDL_CreateRGBSurfaceWithFormat(0,scratchWidth,scratchHeight,32,SDL_PIXELFORMAT_RGBA32);
    }
    if(!g_TextBufferSurface) return;

    finalCopySrc.x = 0;
    finalCopySrc.y = 0;
    finalCopySrc.w = spriteWidth * 2 - 2;
    finalCopySrc.h = fontHeight * 2 - 2;
    if(balanceSeparator) {finalCopySrc.w=spriteWidth*2;finalCopySrc.h=72;}

    SDL_FillRect(g_TextBufferSurface, nullptr, 0);

    if (shadowColor != COLOR_WHITE)
    {
        SDL_Surface *shadowText;

        // Render shadow.
        SDL_Color sdlShadowColor;
        sdlShadowColor.a = 0xFF;
        sdlShadowColor.b = (shadowColor >> 16) & 0xFF;
        sdlShadowColor.g = (shadowColor >> 8) & 0xFF;
        sdlShadowColor.r = shadowColor & 0xFF;

        shadowText = balanceSeparator?RenderStageCaption(g_Font,convertedText,sdlShadowColor):TTF_RenderUTF8_Blended(g_Font, convertedText, sdlShadowColor);

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

    SDL_Color sdlTextColor;
    sdlTextColor.a = 0xFF;
    sdlTextColor.b = (textColor >> 16) & 0xFF;
    sdlTextColor.g = (textColor >> 8) & 0xFF;
    sdlTextColor.r = textColor & 0xFF;

    SDL_Surface *regularText = balanceSeparator?RenderStageCaption(g_Font,convertedText,sdlTextColor):TTF_RenderUTF8_Blended(g_Font, convertedText, sdlTextColor);

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
    if(balanceSeparator) {
        InvertAlpha(0,0,spriteWidth*2,36);
        InvertAlpha(0,40,spriteWidth*2,36);
    } else InvertAlpha(0, 0, spriteWidth * 2, fontHeight * 2 + 6);

    finalCopyDst.x = 0;
    finalCopyDst.y = yPos;
    finalCopyDst.w = spriteWidth;
    finalCopyDst.h = balanceSeparator?36:16;

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
    fonts.clear();fontData.clear();g_Font=nullptr;

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
    auto& texture=g_AnmManager->textures[vm->sprite->sourceFileIndex];
    const int row=vm->sprite->startPixelInclusive.y;
    RenderTextToTexture(0,row,vm->sprite->textureWidth,vm->sprite->textureHeight,
        vm->fontWidth?vm->fontWidth:15,vm->fontHeight?vm->fontHeight:15,color,shadow,text,&texture,true);
    int left=texture.width,right=0;
    if(texture.textureData)for(int y=row;y<std::min<int>(row+36,texture.height);y++)
        for(unsigned x=0;x<texture.width;x++)if(texture.textureData[(y*texture.width+x)*4+3]) {
            left=std::min(left,int(x));right=std::max(right,int(x)+1);
        }
    vm->ctrTextStart=right>left?left:0;vm->ctrTextWidth=std::max(0,right-left);vm->flags.isVisible=true;
}
#endif

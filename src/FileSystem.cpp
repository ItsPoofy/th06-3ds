#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <direct.h>
#include <new>
#include <windows.h>
#elif __cplusplus >= 201703L && !defined(__3DS__)
#include <filesystem>
#else // Assume POSIX
#include <sys/stat.h>
#endif

#include "FileSystem.hpp"
#include "pbg3/Pbg3Archive.hpp"
#include "utils.hpp"
#ifdef __3DS__
#include "platform/3ds/AssetPath.hpp"
#endif

u32 g_LastFileSize;

FILE *FileSystem::FopenUTF8(const char *filepath, const char *mode)
{
#ifndef _WIN32
    FILE* file=std::fopen(filepath, mode);
#ifdef __3DS__
    if(!file && mode[0]=='r' && !std::strchr(filepath, ':')) {
        char rompath[512];std::snprintf(rompath,sizeof(rompath),"sdmc:/3ds/th06/assets/%s",filepath);
        file=std::fopen(rompath,mode);
    }
#endif
    return file;
#else
    u32 filepathWLen = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, NULL, 0) * 2;
    u32 modeWLen = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0) * 2;

    if (filepathWLen == 0 || modeWLen == 0)
    {
        return NULL;
    }

    wchar_t *filepathW = new wchar_t[filepathWLen];
    wchar_t *modeW = new wchar_t[modeWLen];

    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, filepathW, filepathWLen / 2);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, modeW, modeWLen / 2);

    FILE *f = _wfopen(filepathW, modeW);

    delete[] filepathW;
    delete[] modeW;

    return f;
#endif
}

void FileSystem::CreateDir(const char *path)
{
#ifdef _WIN32
    _mkdir(path);
#elif __cplusplus >= 201703L && !defined(__3DS__)
    auto p = std::filesystem::path(path);
    std::filesystem::create_directory(p);
#else
    mkdir(path, 0755);
#endif
}

u8 *FileSystem::OpenPath(const char *filepath, int isExternalResource)
{
    u8 *data;
    FILE *file;
    size_t fsize;
    i32 entryIdx;
    const char *entryname;
    i32 pbg3Idx;

    entryIdx = -1;
#ifdef __3DS__
    if(!isExternalResource) {
        const auto prepared=CtrAssets::PackedPath("data",filepath);
        FILE* unpacked=std::fopen(prepared.c_str(),"rb");
        if(unpacked) {
            std::fseek(unpacked,0,SEEK_END);const long size=std::ftell(unpacked);
            if(size<=0) {std::fclose(unpacked);return nullptr;}
            std::rewind(unpacked);auto* bytes=(u8*)std::malloc(size);
            const bool ok=bytes && std::fread(bytes,1,size,unpacked)==size;
            std::fclose(unpacked);
            if(ok) {g_LastFileSize=size;return bytes;}
            std::free(bytes);return nullptr;
        }
    }
#endif
    if (isExternalResource == 0)
    {
        entryname = std::strrchr(filepath, '\\');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        entryname = std::strrchr(entryname, '/');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        if (g_Pbg3Archives != NULL)
        {
            for (pbg3Idx = 0; pbg3Idx < 0x10; pbg3Idx += 1)
            {
                if (g_Pbg3Archives[pbg3Idx] != NULL)
                {
                    entryIdx = g_Pbg3Archives[pbg3Idx]->FindEntry(entryname);
                    if (entryIdx >= 0)
                    {
                        break;
                    }
                }
            }
        }
        if (entryIdx < 0)
        {
            return NULL;
        }
    }
    if (entryIdx >= 0)
    {
        utils::DebugPrint2("%s Decode ... \n", entryname);
        data = g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry(entryIdx, entryname);
        g_LastFileSize = g_Pbg3Archives[pbg3Idx]->GetEntrySize(entryIdx);
    }
    else
    {
        utils::DebugPrint2("%s Load ... \n", filepath);
        file = FopenUTF8(filepath, "rb");
        if (file == NULL)
        {
            utils::DebugPrint2("error : %s is not found.\n", filepath);
            return NULL;
        }
        else
        {
            std::fseek(file, 0, SEEK_END);
            const long measuredSize=std::ftell(file);
            if(measuredSize<=0) {std::fclose(file);return nullptr;}
            fsize = measuredSize;
            g_LastFileSize = fsize;
            std::fseek(file, 0, SEEK_SET);
            data = (u8 *)std::malloc(fsize);
            const bool loaded=data && std::fread(data, 1, fsize, file)==fsize;
            std::fclose(file);
            if(!loaded) {std::free(data);return nullptr;}
        }
    }
    return data;
}

int FileSystem::WriteDataToFile(const char *path, const void *data, size_t size)
{
    FILE *f;

    f = FopenUTF8(path, "wb");
    if (f == NULL)
    {
        return -1;
    }
    else
    {
        if (std::fwrite(data, 1, size, f) != size)
        {
            std::fclose(f);
            return -2;
        }
        else
        {
            std::fclose(f);
            return 0;
        }
    }
}

#include "ReplayStorage.hpp"
#include "FileSystem.hpp"
#include <cstdio>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

bool ReplayStorage::Write(const char* path,const void* data,std::size_t size) {
    FileSystem::CreateDir("replay");
    const std::string temporary=std::string(path)+".tmp";
    FILE* file=FileSystem::FopenUTF8(temporary.c_str(),"wb");
    if(!file)return false;
    bool ok=std::fwrite(data,1,size,file)==size;
    if(std::fflush(file)!=0)ok=false;
    if(std::fclose(file)!=0)ok=false;
#ifdef _WIN32
    if(ok)ok=MoveFileExA(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    if(ok)ok=std::rename(temporary.c_str(),path)==0;
#endif
    if(!ok)std::remove(temporary.c_str());
    return ok;
}

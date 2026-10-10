#include "VorbisStream.hpp"
#define STB_VORBIS_NO_PUSHDATA_API
#include "thirdparty/stb_vorbis.c"
#include <algorithm>
#include <cstring>
#include <string>
#include <memory>
#include <limits>
#include <new>
#include <map>
#include "AssetPath.hpp"
#include "FileSystem.hpp"
#include "Runtime.hpp"
struct Stream {
    stb_vorbis* decoder=nullptr;
    Sint64 position=0,size=0;
    short buffer[4096];
    size_t cursor=0,bytes=0;
    std::unique_ptr<u8[]> compressed;
    std::unique_ptr<char[]> decoderMemory;
};
static std::map<std::string,SDL_RWops*> prepared;
static std::string MusicPath(const char* input) {
    auto path=CtrAssets::Normalize(input);
    if(auto dot=path.find_last_of('.');dot!=std::string::npos) path.resize(dot);
    return path+".ogg";
}
static Stream* Get(SDL_RWops* rw) { return (Stream*)rw->hidden.unknown.data1; }
static Sint64 Size(SDL_RWops* rw) { return Get(rw)->size; }
static Sint64 Seek(SDL_RWops* rw,Sint64 offset,int whence) {
    auto* s=Get(rw);
    Sint64 p=whence==RW_SEEK_SET?offset:whence==RW_SEEK_CUR?s->position+offset:s->size+offset;
    if(p<0 || p>s->size || p%4) return -1;
    if(!stb_vorbis_seek(s->decoder,p/4)) return -1;
    s->position=p;s->cursor=s->bytes=0;return p;
}
static size_t Read(SDL_RWops* rw,void* out,size_t size,size_t count) {
    if(!size) return 0;
    auto* s=Get(rw);size_t done=0;
    size_t want=std::min<Sint64>(size*count,s->size-s->position);
    while(done<want) {
        if(s->cursor==s->bytes) {
            int n=stb_vorbis_get_samples_short_interleaved(s->decoder,2,s->buffer,4096);
            s->cursor=0;s->bytes=n*4;
            if(!n) break;
        }
        size_t n=std::min(want-done,s->bytes-s->cursor);
        std::memcpy((char*)out+done,(char*)s->buffer+s->cursor,n);
        done+=n;s->cursor+=n;s->position+=n;
    }
    return done/size;
}
static size_t Write(SDL_RWops*,const void*,size_t,size_t) {return 0;}
static int Close(SDL_RWops* rw) {
    auto* s=Get(rw);stb_vorbis_close(s->decoder);delete s;SDL_FreeRW(rw);return 0;
}
static SDL_RWops* Open(const std::string& path) {
    FILE* file=FileSystem::FopenUTF8(path.c_str(),"rb");
    if(!file) return nullptr;
    auto* s=new(std::nothrow) Stream;
    if(!s) {fclose(file);return nullptr;}
    const bool positioned=fseek(file,0,SEEK_END)==0;
    const long fileSize=positioned?ftell(file):-1;
    if(fileSize<=0 || fileSize>std::numeric_limits<int>::max() || fseek(file,0,SEEK_SET)!=0) {
        fclose(file);delete s;return nullptr;
    }
    s->compressed.reset(new(std::nothrow) u8[fileSize]);
    const bool loaded=s->compressed && fread(s->compressed.get(),1,fileSize,file)==size_t(fileSize);
    fclose(file);
    if(!loaded) {delete s;return nullptr;}
    // SDL's 3DS mixer thread has an 80 KiB stack. Keep all decoder setup and
    // frame scratch in one heap arena, also avoiding many small allocations.
    // The shipped stereo tracks require less than 256 KiB including scratch.
    int err=0,arenaBytes=256*1024;
    stb_vorbis* v=nullptr;
    for(;;) {
        s->decoderMemory.reset(new(std::nothrow) char[arenaBytes]);
        if(!s->decoderMemory) break;
        const stb_vorbis_alloc arena{s->decoderMemory.get(),arenaBytes};
        v=stb_vorbis_open_memory(s->compressed.get(),fileSize,&err,&arena);
        if(v || err!=VORBIS_outofmem || arenaBytes>std::numeric_limits<int>::max()/2) break;
        arenaBytes*=2;
    }
    if(!v) {delete s;return nullptr;}
    auto info=stb_vorbis_get_info(v);
    if(info.channels!=2 || info.sample_rate!=44100) {stb_vorbis_close(v);delete s;return nullptr;}
    auto* rw=SDL_AllocRW();
    if(!rw) {stb_vorbis_close(v);delete s;return nullptr;}
    s->decoder=v;s->size=Sint64(stb_vorbis_stream_length_in_samples(v))*4;
    rw->size=Size;rw->seek=Seek;rw->read=Read;rw->write=Write;rw->close=Close;
    rw->type=SDL_RWOPS_UNKNOWN;rw->hidden.unknown.data1=s;return rw;
}
SDL_RWops* OpenVorbisPCM(const char* wavPath) {
    const auto path=MusicPath(wavPath);
    auto found=prepared.find(path);
    if(found!=prepared.end()) {auto* stream=found->second;prepared.erase(found);return stream;}
    return Open(path);
}
void PrepareStageMusic(const char paths[4][128]) {
    for(auto [path,stream]:prepared) SDL_RWclose(stream);
    prepared.clear();
    for(int i=0;i<4;i++) if(std::strchr(paths[i],'.')) {
        const auto path=MusicPath(paths[i]);
        if(prepared.contains(path)) continue;
        if(auto* stream=Open(path)) {
            // Decode the first block now, so the boss cue only exchanges a stream.
            auto* state=Get(stream);
            state->bytes=stb_vorbis_get_samples_short_interleaved(state->decoder,2,state->buffer,4096)*4;
            prepared[path]=stream;
        } else {CtrRuntime::Log("Audio: stage preload failed");CtrRuntime::Log(path.c_str());}
    }
}

#include "ScoreFileValidation.hpp"
#include "ResultScreen.hpp"
#include "GameManager.hpp"
#include <cstring>
namespace {
template<class T> bool ReadRecord(const unsigned char* bytes,std::size_t length,T& out) {
    if(length<sizeof(T))return false;
    std::memcpy(&out,bytes,sizeof(T));return true;
}
}
bool ValidateScoreFile(const ScoreRaw* file,std::size_t available) {
    if(!file || available<sizeof(ScoreRaw))return false;
    ScoreRaw header;std::memcpy(&header,file,sizeof(header));
    if(header.dataOffset<sizeof(ScoreRaw) || header.dataOffset>header.fileLen || header.fileLen>available)return false;
    const auto* bytes=reinterpret_cast<const unsigned char*>(file);
    bool hasHeader=false;
    for(std::size_t offset=header.dataOffset;offset<header.fileLen;) {
        Th6k record;
        if(!ReadRecord(bytes+offset,header.fileLen-offset,record) || record.th6kLen<sizeof(Th6k) || record.th6kLen>header.fileLen-offset)return false;
        if(record.magic==TH6K_MAGIC)hasHeader=true;
        if(record.version==TH6K_VERSION) {
            if(record.magic==HSCR_MAGIC) {
                Hscr run;if(!ReadRecord(bytes+offset,record.th6kLen,run) || run.character>=HSCR_NUM_CHARS_SHOTTYPES || run.difficulty>=HSCR_NUM_DIFFICULTIES)return false;
            } else if(record.magic==PSCR_MAGIC) {
                Pscr practice;if(!ReadRecord(bytes+offset,record.th6kLen,practice) || practice.character>=PSCR_NUM_CHARS_SHOTTYPES || practice.stage>=PSCR_NUM_STAGES || practice.difficulty>=PSCR_NUM_DIFFICULTIES)return false;
            } else if(record.magic==CLRD_MAGIC) {
                Clrd clear;if(!ReadRecord(bytes+offset,record.th6kLen,clear) || clear.characterShotType>=CLRD_NUM_CHARACTERS)return false;
            } else if(record.magic==CATK_MAGIC) {
                Catk spell;if(!ReadRecord(bytes+offset,record.th6kLen,spell) || spell.idx>=CATK_NUM_CAPTURES)return false;
            }
        }
        offset+=record.th6kLen;
    }
    return hasHeader;
}

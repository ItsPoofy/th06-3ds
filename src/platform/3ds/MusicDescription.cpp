#include "MusicDescription.hpp"
#include "TextHelper.hpp"
#include <cstdio>
#include <string>

namespace {
// TextHelper::Measure adds this padding to the advance of the measured text.
constexpr float MeasurePadding=2;

int CharLength(const char* c) {
    const auto byte=static_cast<unsigned char>(*c);
    const bool shiftJisLead=(byte>=0x81 && byte<=0x9f) || (byte>=0xe0 && byte<=0xfc);
    return shiftJisLead && c[1]?2:1;
}

// English rows pad each half-line with spaces. Runs of spaces collapse, and a
// line break becomes a space only between two single-byte characters.
std::string Join(const char (*lines)[66],int lineCount) {
    std::string out;
    bool lastSingleByte=false;
    for(int line=0;line<lineCount;line++) {
        bool lineStart=true,space=false;
        for(const char* c=lines[line];*c;) {
            if(*c==' ') {space=true;c++;continue;}
            const int length=CharLength(c);
            const bool boundarySpace=lineStart && lastSingleByte && length==1;
            if(!out.empty() && ((space && !lineStart) || boundarySpace))out+=' ';
            out.append(c,length);
            lastSingleByte=length==1;lineStart=false;space=false;c+=length;
        }
    }
    return out;
}

float Width(const std::string& text,int fontHeight) {
    return text.empty()?0:TextHelper::Measure(text.c_str(),fontHeight);
}

// Words break at spaces. Double-byte text can break between any two characters.
bool BreakAfter(const std::string& text,size_t end,int lastLength) {
    if(end>=text.size() || text[end]==' ')return true;
    return lastLength==2 || CharLength(&text[end])==2;
}

// Ends of the longest prefix of text[start..] that fits width: the last break
// opportunity, and the last character, each npos when nothing fits.
struct Fit {size_t lastBreak=std::string::npos,lastCharacter=std::string::npos;};
Fit Prefix(const std::string& text,size_t start,int fontHeight,float width) {
    Fit fit;
    for(size_t i=start;i<text.size();) {
        const int length=CharLength(&text[i]);
        const size_t end=i+length;
        if(Width(text.substr(start,end-start),fontHeight)>width)break;
        fit.lastCharacter=end;
        if(BreakAfter(text,end,length))fit.lastBreak=end;
        i=end;
    }
    return fit;
}

// A row ends at its last fitting break. A word wider than the row is cut at
// its last fitting character, and at least one character always advances.
size_t RowEnd(const std::string& text,size_t start,int fontHeight,float width) {
    const Fit fit=Prefix(text,start,fontHeight,width);
    if(fit.lastBreak!=std::string::npos)return fit.lastBreak;
    if(fit.lastCharacter!=std::string::npos)return fit.lastCharacter;
    return start+CharLength(&text[start]);
}

void SetPart(CtrMusicDescription::Part& part,const std::string& text,float x) {
    std::snprintf(part.text,sizeof(part.text),"%s",text.c_str());
    part.x=x;
}

void SplitRow(const std::string& row,int fontHeight,float spriteWidth,CtrMusicDescription::Row& out) {
    out.partCount=1;
    if(Width(row,fontHeight)<=spriteWidth) {SetPart(out.parts[0],row,0);return;}
    // Split at a word when the rest still fits its sprite, otherwise mid-word.
    // The space stays with the first part so the second part's offset includes it.
    const Fit fit=Prefix(row,0,fontHeight,spriteWidth);
    size_t split=fit.lastCharacter;
    if(fit.lastBreak!=std::string::npos) {
        size_t afterSpace=fit.lastBreak;
        while(afterSpace<row.size() && row[afterSpace]==' ')afterSpace++;
        if(Width(row.substr(afterSpace),fontHeight)<=spriteWidth)split=afterSpace;
    }
    const std::string first=row.substr(0,split);
    SetPart(out.parts[0],first,0);
    SetPart(out.parts[1],row.substr(split),Width(first,fontHeight)-MeasurePadding);
    out.partCount=2;
}
}

int CtrMusicDescription::Layout(const char (*lines)[66],int lineCount,int fontHeight,float spriteWidth,float rowWidth,Row* rows,int capacity) {
    const std::string text=Join(lines,lineCount);
    int count=0;
    for(size_t start=0;start<text.size() && count<capacity;) {
        const size_t end=RowEnd(text,start,fontHeight,rowWidth);
        size_t rowEnd=end;
        while(rowEnd>start && text[rowEnd-1]==' ')rowEnd--;
        SplitRow(text.substr(start,rowEnd-start),fontHeight,spriteWidth,rows[count++]);
        start=end;
        while(start<text.size() && text[start]==' ')start++;
    }
    return count;
}

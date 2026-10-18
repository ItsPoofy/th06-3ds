#pragma once
struct ResultScreen;
struct ZunVec3;
namespace CtrRecords {
    inline constexpr int PageSize=5;
    int PageCount();
    void PrepareSpellPage(ResultScreen& screen);
    void DrawSpellPage(ResultScreen& screen,const ZunVec3& origin);
}

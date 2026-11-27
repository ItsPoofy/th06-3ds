#pragma once
namespace CtrRecordHeadings {
bool Prepare();
void Draw(const char* name,float x,float y,unsigned color=0xffffffff,float maxWidth=320,bool centered=false);
float TextWidth(const char* name);
float TextHeight(const char* name);
float CellWidth(const char* name);
float TextAdvance(const char* name);
void DrawCaption(const char* name,float x,float y,bool active,float opacity);
void DrawCaptionAtInk(const char* name,float x,float y,bool active,float opacity);
void DrawText(const char* name,float x,float y,unsigned color=0xffffffff,bool centered=false);
void Release();
}

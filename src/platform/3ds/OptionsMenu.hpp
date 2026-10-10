#pragma once
#include <string>
struct MainMenu;
namespace CtrOptions {
inline constexpr float RowsTop=38,RowSpacing=27;
inline constexpr float CategoriesTop=48,CategorySpacing=28;
void Open();
int Page();int RowCount();int Update(MainMenu& menu);bool Adjustable(int row);
int PageCount();int ControlsRow();
const char* Heading(int page);
const char* Label(int row);
std::string Value(int row);
}

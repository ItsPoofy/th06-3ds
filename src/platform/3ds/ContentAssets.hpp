#pragma once
#include <string>
#include "AssetRoot.hpp"
namespace CtrContent {
std::string Path(const char* directory,const char* name);
const char* Translate(const char* utf8);
}

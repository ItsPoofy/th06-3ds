#pragma once
#include <string>
#include <algorithm>
#include "ContentAssets.hpp"

namespace CtrAssets {
inline std::string Normalize(const char* path) {
    std::string result(path);
    std::replace(result.begin(),result.end(),'\\','/');
    return result;
}
inline std::string PackedPath(const char* directory,const char* path) {
    auto name=Normalize(path);
    name=name.substr(name.find_last_of('/')+1);
    return CtrContent::Path(directory,name.c_str());
}
}

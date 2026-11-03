#pragma once
#include <cstddef>
namespace ReplayStorage {
// Commit a completed replay without truncating the previous save on failure.
bool Write(const char* path,const void* data,std::size_t size);
}

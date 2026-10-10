#pragma once
#include <cstddef>
struct ScoreRaw;
// Validate decrypted records before any native reader follows their offsets.
bool ValidateScoreFile(const ScoreRaw* file,std::size_t available);

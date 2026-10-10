import sys
from pathlib import Path
data=Path(sys.argv[1]).read_bytes()
Path(sys.argv[2]).write_text('#pragma once\n#include <cstdint>\nalignas(4) static const uint8_t scene_shader[] = {'+','.join(map(str,data))+'};\n')

#pragma once
#include <stdint.h>
namespace raum {

struct Size {
    uint32_t width{0};
    uint32_t height{0};
};

struct Position {
    int32_t x{0};
    int32_t y{0};
};

} // namespace raum
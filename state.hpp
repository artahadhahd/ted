#ifndef ED_STATE_HPP
#define ED_STATE_HPP
#include "file.hpp"
#include <cstdint>

namespace editor
{
    extern struct State {
        int32_t key;
        size_t maxy, maxx;
        size_t cursory, cursorx;
        size_t buffer_cursor;
        size_t line;
    } state;
}

#endif
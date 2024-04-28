#ifndef ED_STATE_HPP
#define ED_STATE_HPP
#include "file.hpp"
#include <cstdint>


namespace editor
{
    extern struct State {
        int32_t key;
        size_t maxy, maxx;
        size_t cursory;
        size_t buffer_cursor;
        size_t line;
        size_t offset;

        size_t cursx() const {
            return buffer_cursor + offset;
        }

        size_t y() const {
            return line + cursory;
        }
    } state;
}

#endif
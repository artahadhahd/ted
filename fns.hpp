#pragma once
#include "state.hpp"
#include <curses.h>

namespace editor {
    void getchar();
    void exit_handler();
    void initialize();
}
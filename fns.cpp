#include "fns.hpp"

namespace editor {
    void getchar()
    {
        editor::state.key = wgetch(stdscr);
    }

    void exit_handler()
    {
        endwin();
    }

    void initialize()
    {
        initscr();
        noecho();
        nonl();
        keypad(stdscr, true);
        // curs_set(0);
        raw();
        cbreak(); // temporary
        editor::state.maxx = getmaxx(stdscr);
        editor::state.maxy = getmaxy(stdscr);
    }
}
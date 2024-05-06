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
        if (initscr() == NULL) {
            std::cerr << "Couldn't initialize ncurses in " << __PRETTY_FUNCTION__ << std::endl;
            exit(1);
        }
        noecho();
        nonl();
        keypad(stdscr, true);
        raw();
        start_color();
        init_color(COLOR_YELLOW, 600, 600, 100);
        init_pair(LINE_COLOR, COLOR_YELLOW, COLOR_BLACK);
        editor::state.maxx = getmaxx(stdscr);
        editor::state.maxy = getmaxy(stdscr);
    }
}
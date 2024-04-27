#include "editor.hpp"
#include <cstdlib>
#include <curses.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <sys/ioctl.h>

namespace fs = std::filesystem;

constexpr inline int ctrl(int const key) { 
    return key & 0x1F;
}

constexpr const int ESCAPE_KEY = 27;
constexpr const int ENTER_KEY = 13;
constexpr const int BACKSPACE = 127;

void terminal_resize(int sig)
{
    using editor::state;
    (void)sig;
    struct winsize size;
    if (ioctl(0, TIOCGWINSZ, &size) < 0) {
        exit(2);
    }
    state.maxy = size.ws_row;
    state.maxx = size.ws_col;
    state.cursory = std::min(state.cursory, state.maxy);
}


int main(int argc, char* argv[])
{
    using editor::state;
    if (std::atexit(editor::exit_handler) != 0) {
        std::cerr << "Couldn't set up editor properly" << std::endl;
        return 1;
    }

    if (argc != 2) {
        std::cerr << "Invalid amount of args: " << argc << std::endl;
        exit(1);
    }
    // std::string subject = argv[1];
    // fs::path path = fs::path(argv[1]);
    // std::cout << path.extension().c_str();
    // editor::initialize();
    fs::path path(argv[1]);
    // std::cout << path.empty() << '\n';
    if (!fs::is_regular_file(path)) {
        std::cout << path << " is not a file" << std::endl;
        exit(1);
    }

    struct sigaction winch_handler;
    winch_handler.sa_handler = terminal_resize;
    signal(SIGWINCH, winch_handler.sa_handler);    

    editor::initialize();
    editor::File file(path);
    auto buffer = file.content();
    while (1) {
        mvprintw(0, 0, "%s", buffer.c_str());
        move(state.cursory, state.cursorx);
        editor::getchar();
        if (state.key == ctrl('q') || state.key == ESCAPE_KEY) {
            break;
        }
        else if (state.key == '>') {
            size_t cursor = state.cursorx;
            while (state.cursorx < buffer.size()) {
                if (buffer.at(state.cursorx++) == '\n') {
                    break;
                }
            }
            if (buffer[state.cursorx] == '\n') {
                state.cursory++;
                state.cursorx = 0;
            } else {
                state.cursorx = cursor;
            }
            // exit(3);
        }
        else if (state.key == ctrl('l')) {
            if (state.cursorx < buffer.size()) {
                if (buffer.at(state.buffer_cursor) != '\n') {
                    ++state.cursorx;
                    ++state.buffer_cursor;
                }
            }
        }
        else if (state.key == ctrl('h')) {
            if (state.buffer_cursor > 0) {
                if (state.cursorx > 0)
                    state.cursorx--;
                state.buffer_cursor--;
            }
        }
        else if (state.key == KEY_BACKSPACE) {
            if (state.buffer_cursor > 0) {
                buffer.erase(--state.buffer_cursor, 1);
                --state.cursorx;
            }
        }
        else if (state.key == KEY_DC) {
            buffer.erase(state.buffer_cursor, 1);
        }
        else {
            char ch = state.key == ENTER_KEY ? '\n' : char(state.key);
            buffer.insert(state.buffer_cursor, 1, ch);
            if (ch == '\n') {
                // state.buffer_cursor++;
                // state.cursory++;
            } else {
                state.buffer_cursor++;
                state.cursorx++;
            }
        }
        clear();
    }
}

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

// signature has to be like this because you have to pass a function pointer
// somewhere else in the code.
void terminal_resize(int)
{
    using editor::state;
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

    fs::path path(argv[1]);
    if (!fs::is_regular_file(path)) {
        std::cout << path << " is not a file" << std::endl;
        exit(1);
    }

    struct sigaction winch_handler;
    winch_handler.sa_handler = terminal_resize;
    signal(SIGWINCH, winch_handler.sa_handler);    

    editor::initialize();
    editor::File file(path);
    auto buffer = file.content_buffered();
    bool save = false;
    while (1) {
        // render
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (i >= state.maxy) break;
            if (state.line + i >= buffer.size()) break;
            if (state.offset) {
                attron(COLOR_PAIR(LINE_COLOR));
                mvprintw(i, 0, "%zu", i + 1 + state.line);
                attroff(COLOR_PAIR(LINE_COLOR));
            }
            mvprintw(i, editor::state.offset, "%s", buffer.at(state.line + i).c_str());
        }

        move(state.cursory, state.cursx());
        editor::getchar();
        if (state.key == ctrl('q')) {
            clear();
            mvprintw(0, 0, "Save modifier buffer? (y/n/c)");
            refresh();
            editor::getchar();
            if (state.key == 'y') {
                save = true;
                break;
            } else if (state.key == 'n') {
                break;
            }
            clear();
            refresh();
            continue;
        }
        if (state.key == ctrl('e')) {
            if (state.y() < buffer.size() - 1) {
                state.line++;
                state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.line).size());
            }
        }
        if (state.key == ctrl('r')) {
            if (state.line != 0) {
                state.line--;
                state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.line).size());
            }
        }
        
        if (state.key == ctrl('n')) {
            if (state.offset == 0)
                state.offset = 6;
            else
                state.offset = 0;
        }

        if (state.key == ctrl('l')) {
            if (state.buffer_cursor < buffer.at(state.y()).size()) {
                ++state.buffer_cursor;
            }
        }

        if (state.key == ctrl('h')) {
            if (state.buffer_cursor > 0) {
                --state.buffer_cursor;
            }
        }

        if (state.key == ctrl('k')) {
            if (state.cursory > 0) {
                --state.cursory;
            } else if (state.line > 0) {
                --state.line;
            }
            state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.y()).size());
        }

        if (state.key == ctrl('j')) {
            if (state.y() + 1 < buffer.size()) {
                if (state.cursory != state.maxy - 1) {
                    state.cursory++;
                } else {
                    state.line++;
                }
                state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.y()).size());
            }
        }

        if (isprint(state.key)) {
            buffer.at(state.y()).insert(state.buffer_cursor++, 1, char(state.key));
            // buffer.at(state.line).at(state.buffer_cursor).insert(state.buffer_cursor, 1, state.key);
        }

        // maybe will find a more elegant/efficient solution but this works
        if (state.key == ENTER_KEY || state.key == KEY_ENTER) {
            if (buffer.at(state.y()).size() == 0) {
                buffer.insert(buffer.begin() + state.y() + 1, 1, "");
            } else {
                buffer.at(state.y()).insert(state.buffer_cursor, 1, '\n');
                std::stringstream ss;
                ss << buffer.at(state.y());
                std::string l;
                std::getline(ss, l);
                buffer[state.y()] = l;
                std::getline(ss, l);
                buffer.insert(buffer.begin() + state.y() + 1, 1, l);
            }
            state.buffer_cursor = 0;
            state.cursory++;
        }

        if (state.key == KEY_BACKSPACE || state.key == BACKSPACE) {
            if (state.buffer_cursor > 0) {
                buffer.at(state.y()).erase(--state.buffer_cursor, 1);
            }
        }

        if (state.key == KEY_DC) {
            if (buffer.at(state.y()).size() != 0)
                buffer.at(state.y()).erase(state.buffer_cursor, 1);
            else if (buffer.size() != 0)
                buffer.erase(buffer.begin() + state.y());
        }

        refresh();
        clear();
    }
    if (save) {
        std::ofstream f(path);
        if (!f.is_open()) {
            exit(3);
        }
        for (size_t i = 0; i < buffer.size(); ++i) {
            f << buffer.at(i) << '\n';
        }
        f.close();
    }
}

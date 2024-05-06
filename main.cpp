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

// signature has to be like this because you have to pass a function pointer (to C code)
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

    enum class RenderMode : char {
        // no need to re=render/refresh (when just moving around)
        None = 0,
        // modify single line (edit mode), only re-render one line.
        Single = 1,
        // clear the screen and re-render
        Screen = 2,
    } renderState = RenderMode::Screen; // set to screen because we need to render the whole thing first

    editor::initialize();
    editor::File file(path);
    auto buffer = file.content_buffered();
    if (buffer.size() == 0) {
        buffer.push_back("");
    }
    bool save = false;
    while (1) {
        // render
        if (renderState == RenderMode::Screen) {
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
        } else if (renderState == RenderMode::Single) {
            move(state.cursory, 0);
            clrtoeol();
            if (state.offset) {
                attron(COLOR_PAIR(LINE_COLOR));
                mvprintw(state.cursory, 0, "%zu", state.cursory + 1 + state.line);
                attroff(COLOR_PAIR(LINE_COLOR));
            }
            mvprintw(state.cursory, editor::state.offset, "%s", buffer.at(state.y()).c_str());
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
            renderState = RenderMode::Screen;
            clear();
            refresh();
            continue;
        }
        if (state.key == ctrl('e')) {
            if (state.y() < buffer.size() - 1) {
                state.line++;
                state.buffer_cursor = 0;
                renderState = RenderMode::Screen;
            }
        }
        if (state.key == ctrl('r')) {
            if (state.line != 0) {
                state.line--;
                // state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.line).size());
                state.buffer_cursor = 0;
                renderState = RenderMode::Screen;
            }
        }
        
        if (state.key == ctrl('n')) {
            if (state.offset == 0)
                state.offset = 6;
            else
                state.offset = 0;
            renderState = RenderMode::Screen;
        }

        if (state.key == ctrl('l') || state.key == KEY_RIGHT) {
            if (state.buffer_cursor < buffer.at(state.y()).size()) {
                ++state.buffer_cursor;
            }
            renderState = RenderMode::None;
        }

        if (state.key == ctrl('h') || state.key == KEY_LEFT) {
            if (state.buffer_cursor > 0) {
                --state.buffer_cursor;
            }
            renderState = RenderMode::None;
        }

        if (state.key == ctrl('s')) {
            std::ofstream f(path);
            if (!f.is_open()) {
                exit(3);
            }
            for (size_t i = 0; i < buffer.size(); ++i) {
                f << buffer.at(i) << '\n';
            }
            f.close();
        }

        if (state.key == ctrl('k') || state.key == KEY_UP) {
            if (state.cursory > 0) {
                --state.cursory;
                renderState = RenderMode::None;
            } else if (state.line > 0) {
                --state.line;
                renderState = RenderMode::Screen;
            }
            state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.y()).size());
        }

        if (state.key == ctrl('j') || state.key == KEY_DOWN) {
            if (state.y() + 1 < buffer.size()) {
                if (state.cursory != state.maxy - 1) {
                    state.cursory++;
                    renderState = RenderMode::None;
                } else {
                    state.line++;
                    renderState = RenderMode::Screen;
                }
                state.buffer_cursor = std::min(state.buffer_cursor, buffer.at(state.y()).size());
            }
        }

        if (isprint(state.key)) {
            buffer.at(state.y()).insert(state.buffer_cursor++, 1, char(state.key));
            renderState = RenderMode::Single;
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
            renderState = RenderMode::Screen;
        }

        if (state.key == KEY_BACKSPACE || state.key == BACKSPACE) {
            if (state.buffer_cursor > 0) {
                buffer.at(state.y()).erase(--state.buffer_cursor, 1);
            }
            renderState = RenderMode::Single;
        }

        if (state.key == ctrl('d')) {
            state.buffer_cursor = 0;
            clrtoeol();
            buffer.at(state.y()) = "";
            renderState = RenderMode::Single;
        }

        if (state.key == KEY_DC) {
            if (buffer.size() > 0) {
                if (buffer.at(state.y()).size() != 0)
                    buffer.at(state.y()).erase(state.buffer_cursor, 1);
                else if (buffer.size() != 0)
                    buffer.erase(buffer.begin() + state.y());
            }
            renderState = RenderMode::Single;
        }

        if (state.key == ctrl(' ')) {
            if (buffer.at(state.y()).size() > state.buffer_cursor) {
                while (isblank(buffer.at(state.y()).at(state.buffer_cursor))) {
                    ++state.buffer_cursor;
                }
                renderState = RenderMode::None;
            }
        }

        if (state.key == ctrl('a')) {
            editor::getchar();
            if (state.key == ctrl('l')) {
                auto const size = buffer.at(state.y()).size();
                state.buffer_cursor = size != 0 ? size : 1;
            } else if (state.key == ctrl('h')) {
                state.buffer_cursor = 0;
            } else if (state.key == ctrl('j')) {
                auto const size = buffer.size() - 1;
                state.line = size != 0 ? size : 1;
                state.cursory = 0;
            } else if (state.key == ctrl('k')) {
                state.cursory = state.line = 0;
            }
            renderState = RenderMode::Single;
        }

        // if (state.key == ctrl(KEY_BACKSPACE)) {
        //     exit(8);
        // }

        refresh();
        if (renderState == RenderMode::Screen) {
            clear();
        }
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

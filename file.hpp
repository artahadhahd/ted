#pragma once
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define LINE_COLOR 1

namespace editor {

struct File {
    std::fstream file;
    std::filesystem::path path;
    
    File(std::filesystem::path file) : path(file) {
        this->file.open(file);
    }

    ~File() {
        file.close();
    }

    std::string content() {
        std::stringstream sstream;
        sstream << file.rdbuf();
        return sstream.str();
    }

    std::vector<std::string> content_buffered() {
        std::vector<std::string> buffer;
        std::string line {};
        while (std::getline(file, line)) {
            buffer.push_back(line);
        }
        return buffer;
    }
};

}
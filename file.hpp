#pragma once
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace editor {

struct File {
    std::fstream file;
    std::filesystem::path path;

    File(){}

    File(std::filesystem::path file) : path(file) {
        this->file.open(file);
    }

    ~File() {
        std::cout << "Closing file " << path << std::endl;
        file.close();
    }

    void write(std::string_view buffer) {
        if (file.is_open()) {
            file << buffer;
        }
    }

    void append(std::string_view buffer) {
        if (file.is_open()) {
            file.close();
        }
        file.open(path, std::ios_base::app);
        file << buffer;
        file.close();
        file.open(path);
    }

    std::string content() {
        // std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        // return content;
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
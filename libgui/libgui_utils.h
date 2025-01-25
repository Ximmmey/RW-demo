#pragma once

#include <fstream>
#include <sstream>
#include <string>

namespace libgui {

    /**
     * @brief copies a file to a string
     * @param path File to read
     * @return Copied data
     */
static std::string read_file_to_str(const char* path) {
    const std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

}

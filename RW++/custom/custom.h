#pragma once

#include <glm/vec2.hpp> // Include glm::vec2
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp> // Include GLM library

namespace custom
{
    /*
    glm::ivec2 getTilePos(const glm::vec2& position) {
        // Offset to ensure (0,0) maps to the center of the first tile
        glm::vec2 offset(10.0f, 10.0f);

        // Apply offset and scale down to grid
        glm::vec2 adjustedPosition = (position + offset) / 20.0f;

        return glm::ivec2(glm::floor(adjustedPosition));
    }

    int getTilePos(float value) {
        float offset = 10.0f;

        return static_cast<int>(std::floor((value + offset) / 20.0f));
    }
    */

    int getTilePos(float value) {
        return static_cast<int>(value / 20.0f);
    }

    glm::ivec2 getTilePos(const glm::vec2& position) {
        glm::ivec2 result;
        result.x = getTilePos(position.x);
        result.y = getTilePos(position.y);
        return result;
    }

    //Cam coordinates are contained on 4th line, for now we assume there is only one camera
    static glm::ivec2 getCamOffsetFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filepath);
        }

        std::string line;
        int currentLine = 0;

        while (std::getline(file, line)) {
            ++currentLine;
            if (currentLine == 4) {
                std::stringstream ss(line);
                int x, y;
                char delimiter;

                if (ss >> x >> delimiter >> y && delimiter == ',') {
                    printf("Camoffset.x = %d\n", x);
                    printf("Camoffset.y = %d\n", y);
                    return glm::ivec2(x, y);
                } else {
                    throw std::runtime_error("Invalid format on line 4: " + line);
                }
            }
        }

        throw std::runtime_error("File has fewer than 4 lines: " + filepath);
    }
}
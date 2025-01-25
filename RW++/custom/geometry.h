#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include "matrix.h"
#include <glm/glm.hpp>
#include <glm/vec2.hpp> // Include glm::vec2

#include "custom.h"

class RoomGeometry {
public:
    RoomGeometry(int x_size, int y_size, const std::vector<int>& tiles)
        : x_size(x_size), y_size(y_size), grid(x_size, y_size) {
        if (tiles.size() != static_cast<size_t>(x_size * y_size)) {
            throw std::invalid_argument("Tile data size does not match room dimensions.");
        }

        int index = 0;
        for (int y = 0; y < y_size; ++y) {
            for (int x = 0; x < x_size; ++x) {
                grid.at(x, y) = tiles[index++];
            }
        }
    }

    //Data we care for is contained on 2nd and 12th lines
    static RoomGeometry fromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening file: " + filepath);
        }

        std::string line;
        int x_size = 0, y_size = 0;

        // Read the second line for XSize and YSize
        for (int i = 0; i < 2; ++i) {
            std::getline(file, line);
            if (i == 1) {
                std::stringstream ss(line);
                std::string part;
                if (std::getline(ss, part, '*')) {
                    x_size = std::stoi(part);
                    //printf("X size is: %d\n", x_size);
                }
                if (std::getline(ss, part, '|')) {
                    y_size = std::stoi(part);
                    //printf("Y size is: %d\n", y_size);
                }
            }
        }

        // Skip to the 12th line
        for (int i = 2; i < 12; ++i) {
            std::getline(file, line);
        }

        // Parse the array on the 12th line

        //printf("Line to parse: %s\n", line.c_str());
        //printf("Parsing tiles:\n"); 

        std::vector<int> tiles = parseLine(line);

        //printf("Total tiles read: %zu\n", tiles.size());

        tiles = reorderVector(tiles, x_size, y_size);

        return RoomGeometry(x_size, y_size, tiles);
    }

    int getTileType(int x, int y) const {
        if (x < 0) {
            x = 0;
        } else if (x >= x_size) {
            x = x_size - 1;
        }

        if (y < 0) {
            y = 0;
        } else if (y >= y_size) {
            y = y_size - 1;
        }

        y = y_size - y - 1;

        return grid.at(x, y);
    }

    int getTileType(glm::ivec2 pos){
        return getTileType(pos.x, pos.y);
    }

    int getTileType(glm::vec2 pos){
        return getTileType(custom::getTilePos(pos));
    }

    int getXSize() const { return x_size; }
    int getYSize() const { return y_size; }

    void printGrid() const {
        grid.print();
    }

private:
    int x_size, y_size;
    Matrix grid;

    //I blame lingo :(<
    static std::vector<int> parseLine(const std::string& line) {
        std::vector<int> result;
        std::stringstream ss(line);
        std::string cell;

        // Split the line by '|'
        while (std::getline(ss, cell, '|')) {
            bool hasOne = false;
            std::stringstream cellStream(cell);
            std::string number;

            // Split the cell by ','
            while (std::getline(cellStream, number, ',')) {
                if (std::stoi(number) == 1) {
                    hasOne = true;
                    break;
                }
            }

            // Append 1 if '1' is found, otherwise 0
            result.push_back(hasOne ? 1 : 0);
        }

        return result;
    }

    //Since lingo is major first (and so, saved geometry is a column by column in a single line), we need to transpose the geometry
    static std::vector<int> reorderVector(const std::vector<int>& columnMajor, int x_size, int y_size) {
        std::vector<int> rowMajor(x_size * y_size);

        for (int row = 0; row < y_size; ++row) {
            for (int col = 0; col < x_size; ++col) {
                // Convert column-major index to row-major index
                rowMajor[row * x_size + col] = columnMajor[col * y_size + row];
            }
        }

        return rowMajor;
    }

};
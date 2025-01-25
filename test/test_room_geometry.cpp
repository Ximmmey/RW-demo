#include <gtest/gtest.h>
#include "geometry.h"
#include "custom.h"
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

// Test for RoomGeometry Constructor with valid data
TEST(RoomGeometryTest, ConstructorValid) {
    std::vector<int> tiles = {1, 0, 1, 1, 0, 1};

    //1 0 1
    //1 0 1

    RoomGeometry room(3, 2, tiles);

    EXPECT_EQ(room.getXSize(), 3);
    EXPECT_EQ(room.getYSize(), 2);
    EXPECT_EQ(room.getTileType(0, 0), 1);
    EXPECT_EQ(room.getTileType(1, 1), 0);
}

// Test for RoomGeometry Constructor with invalid tile data size
TEST(RoomGeometryTest, ConstructorInvalidTileSize) {
    std::vector<int> tiles = {1, 0}; // Invalid size, should throw
    EXPECT_THROW(RoomGeometry room(3, 2, tiles), std::invalid_argument);
}

// Test for RoomGeometry fromFile with valid file
TEST(RoomGeometryTest, FromFileValid) {
    // Create a valid test file
    std::ofstream file("test_room.txt");
    file << "Some initial lines\n";
    file << "3*3|3\n";  // Room dimensions
    file << "2\n";
    file << "Some more lines\n";
    file << "3\n";
    file << "1\n";
    file << "2\n";
    file << "3\n";
    file << "4\n";
    file << "5\n";
    file << "0|1|0|1|0|1|0|1|0\n";  // Tile data
    //0 1 0
    //1 0 1
    //0 1 0
    file << "6\n";
    file.close();

    RoomGeometry room = RoomGeometry::fromFile("test_room.txt");

    // Test that the room was correctly parsed
    EXPECT_EQ(room.getXSize(), 3);
    EXPECT_EQ(room.getYSize(), 3);
    EXPECT_EQ(room.getTileType(0, 0), 0); 
    EXPECT_EQ(room.getTileType(1, 1), 0); 

    // Clean up the file after test
    std::remove("test_room.txt");
}

// Test for RoomGeometry fromFile with invalid file
TEST(RoomGeometryTest, FromFileInvalidFile) {
    // Try loading an invalid file (non-existent)
    EXPECT_THROW(RoomGeometry::fromFile("invalid_file.txt"), std::runtime_error);
}

// Test for getTileType with valid coordinates
TEST(RoomGeometryTest, GetTileTypeValid) {
    std::vector<int> tiles = {1, 0, 1, 1, 0, 1};
    //1 0 1
    //1 0 1
    RoomGeometry room(3, 2, tiles);

    // Using int-based coordinates
    EXPECT_EQ(room.getTileType(0, 0), 1);
    EXPECT_EQ(room.getTileType(2, 1), 1); 

    EXPECT_EQ(room.getTileType(glm::ivec2(1, 1)), 0);

    EXPECT_EQ(room.getTileType(glm::vec2(1.0f, 1.0f)), 1); // friendly reminder : each int tile corresponds to 20 float pixels
}

// Test for getTileType with out-of-bounds coordinates
TEST(RoomGeometryTest, GetTileTypeOutOfBounds) {
    std::vector<int> tiles = {1, 0, 1, 1, 0, 1};
    //1 0 1
    //1 0 1
    RoomGeometry room(3, 2, tiles);

    // Test out of bounds behavior
    EXPECT_EQ(room.getTileType(-1, -1), 1);
    EXPECT_EQ(room.getTileType(5, 5), 1);
}

// Test for printGrid method
TEST(RoomGeometryTest, PrintGrid) {
    std::vector<int> tiles = {1, 0, 1, 1, 0, 1};
    RoomGeometry room(3, 2, tiles);

    // Redirect stdout to capture the output of printGrid
    std::stringstream ss;
    std::streambuf* old_buffer = std::cout.rdbuf(ss.rdbuf());

    room.printGrid(); // Call printGrid() to output to the stringstream

    // Check if output contains the expected grid structure
    std::string expected_output = "1 0 1 \n1 0 1 \n"; // Modify according to your print format
    EXPECT_EQ(ss.str(), expected_output);

    // Restore the original buffer
    std::cout.rdbuf(old_buffer);
}

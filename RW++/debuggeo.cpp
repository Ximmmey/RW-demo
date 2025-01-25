//This is evil and slow, don't use

class SceneDebugGeo {
    std::vector<glm::vec2> positions;

    // Debug controls
    bool displaying = false;

public:
    explicit SceneDebugGeo(const RoomGeometry& roomGeo, glm::vec2 offset) {
        // Get tiles and cache them
        int x_size = roomGeo.getXSize();
        int y_size = roomGeo.getYSize();

        offset.x = -offset.x;
        offset.y = offset.y;

        for (int y = 0; y < y_size; ++y) {
            for (int x = 0; x < x_size; ++x) {
                if (roomGeo.getTileType(x, y) == 1) {
                    positions.push_back(glm::vec2(x * 20.0f, 760 - y * 20.0f) + offset);
                }
            }
        }
    }

    void frame_update() {
        if (displaying) {
            const auto dl = ImGui::GetBackgroundDrawList();
            for (const auto pos: positions) {
                dl->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + 20, pos.y - 20), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 0, 0, 2);
            }
        }

        ImGui::Begin("Debug Geometry");

        ImGui::Checkbox("Display", &displaying);

        ImGui::End();
    }
};

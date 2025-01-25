#include "rendering.h"

#include <vector>

void DrawPoller::reset() {
    Descriptions.clear();
}

void DrawPoller::make_custom(const RenderDescription &desc) {
    Descriptions.push_back(desc);
}

template<typename T>
void DrawPoller::make_sprite(const glm::vec2 pos, const glm::vec2 size, const float depth, const float scale, const VkDescriptorSet scene_set, const VkDescriptorSet object_set, std::shared_ptr<T> uniform) {
    const float half_w = (size.x / 2) * scale;
    const float half_h = (size.y / 2) * scale;

    constexpr glm::vec4 white{1};

    const glm::vec3 top_right{pos.x + half_w, pos.y + half_h, depth};
    const glm::vec3 top_left{pos.x - half_w, pos.y + half_h, depth};
    const glm::vec3 bottom_left{pos.x - half_w, pos.y - half_h, depth};
    const glm::vec3 bottom_right{pos.x + half_w, pos.y - half_h, depth};

    std::vector vertices = {
        Vertex(top_right, {1, 0}, white),
        Vertex(top_left, {0, 0}, white),
        Vertex(bottom_left, {0, 1}, white),
        Vertex(bottom_right, {1, 1}, white),
    };

    std::vector<uint16_t> indices = {
        0,
        1,
        2,
        3,
        0,
        2,
    };

    RenderDescription desc{
        .mesh_vertices = vertices,
        .mesh_indices = indices,
        .scene_set = scene_set,
        .object_set = object_set,
        .uniform = uniform,
        .uniform_size = sizeof(T),
    };

    Descriptions.push_back(std::move(desc));
}

void DrawPoller::make_sprite(const glm::vec2 pos, const glm::vec2 size, const float depth, const float scale, const VkDescriptorSet scene_set, const VkDescriptorSet object_set) {
    const float half_w = (size.x / 2) * scale;
    const float half_h = (size.y / 2) * scale;

    constexpr glm::vec4 white{1};

    const glm::vec3 top_right{pos.x + half_w, pos.y + half_h, depth};
    const glm::vec3 top_left{pos.x - half_w, pos.y + half_h, depth};
    const glm::vec3 bottom_left{pos.x - half_w, pos.y - half_h, depth};
    const glm::vec3 bottom_right{pos.x + half_w, pos.y - half_h, depth};

    std::vector vertices = {
        Vertex(top_right, {1, 1}, white),
        Vertex(top_left, {0, 1}, white),
        Vertex(bottom_left, {0, 0}, white),
        Vertex(bottom_right, {1, 0}, white),
    };

    std::vector<uint16_t> indices = {
        0,
        1,
        2,
        3,
        0,
        2,
    };

    RenderDescription desc{
        .mesh_vertices = vertices,
        .mesh_indices = indices,
        .scene_set = scene_set,
        .object_set = object_set,
        .uniform = nullptr,
        .uniform_size = 0,
    };

    Descriptions.push_back(std::move(desc));
}

template<typename T>
RenderDescription DrawPoller::cache_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set, std::shared_ptr<T> uniform) {
    const float half_w = (size.x / 2) * scale;
    const float half_h = (size.y / 2) * scale;

    constexpr glm::vec4 white{1};

    const glm::vec3 top_right{pos.x + half_w, pos.y + half_h, depth};
    const glm::vec3 top_left{pos.x - half_w, pos.y + half_h, depth};
    const glm::vec3 bottom_left{pos.x - half_w, pos.y - half_h, depth};
    const glm::vec3 bottom_right{pos.x + half_w, pos.y - half_h, depth};

    std::vector vertices = {
        Vertex(top_right, {1, 0}, white),
        Vertex(top_left, {0, 0}, white),
        Vertex(bottom_left, {0, 1}, white),
        Vertex(bottom_right, {1, 1}, white),
    };

    std::vector<uint16_t> indices = {
        0,
        1,
        2,
        3,
        0,
        2,
    };

    return RenderDescription {
        .mesh_vertices = vertices,
        .mesh_indices = indices,
        .scene_set = scene_set,
        .object_set = object_set,
        .uniform = uniform,
        .uniform_size = sizeof(T),
    };
}

RenderDescription DrawPoller::cache_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set) {
    const float half_w = (size.x / 2) * scale;
    const float half_h = (size.y / 2) * scale;

    constexpr glm::vec4 white{1};

    const glm::vec3 top_right{pos.x + half_w, pos.y + half_h, depth};
    const glm::vec3 top_left{pos.x - half_w, pos.y + half_h, depth};
    const glm::vec3 bottom_left{pos.x - half_w, pos.y - half_h, depth};
    const glm::vec3 bottom_right{pos.x + half_w, pos.y - half_h, depth};

    std::vector vertices = {
        Vertex(top_right, {1, 1}, white),
        Vertex(top_left, {0, 1}, white),
        Vertex(bottom_left, {0, 0}, white),
        Vertex(bottom_right, {1, 0}, white),
    };

    std::vector<uint16_t> indices = {
        0,
        1,
        2,
        3,
        0,
        2,
    };

    return RenderDescription {
        .mesh_vertices = vertices,
        .mesh_indices = indices,
        .scene_set = scene_set,
        .object_set = object_set,
        .uniform = nullptr,
        .uniform_size = 0,
    };
}

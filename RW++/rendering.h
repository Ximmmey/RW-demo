#pragma once

#include "textures.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vector>

// Vertex data
struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec4 color;
};

// Mesh data
struct Mesh {
    std::span<Vertex> vertices;
    std::span<uint32_t> indices;
};

// Mesh buffers for the scene
struct MeshBuffers {
    libgui::VkSizedBuffer vertices;
    libgui::VkSizedBuffer indices;
};

// Standard per-frame render description
struct RenderDescription {
    std::vector<Vertex> mesh_vertices;
    std::vector<uint16_t> mesh_indices;
    VkDescriptorSet scene_set;
    VkDescriptorSet object_set;

    std::shared_ptr<void> uniform;
    uint32_t uniform_size;
};

// Poller belonging to a pipeline, records render descs
class DrawPoller {
public:
    std::vector<RenderDescription> Descriptions;

    void reset();

    void make_custom(const RenderDescription &desc);

    template<typename T>
    void make_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set, std::shared_ptr<T> uniform);

    void make_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set);

    template<typename T>
    static RenderDescription cache_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set, std::shared_ptr<T> uniform);

    static RenderDescription cache_sprite(glm::vec2 pos, glm::vec2 size, float depth, float scale, VkDescriptorSet scene_set, VkDescriptorSet object_set);
};

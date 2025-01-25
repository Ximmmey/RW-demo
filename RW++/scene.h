#pragma once

#include "rendering.h"
#include "pipelines.h"

#include <libgui_vkutils.h>
#include <cstdint>
#include <memory>

class Scene;

// Interface for scene objects
class SceneObject_T {
public:
    virtual ~SceneObject_T() = default;

    virtual void physics_tick(Scene *scene) = 0;
    virtual void frame_update(Scene *scene) = 0;

    virtual void poll_draw() = 0;
};

// In-Scene scene object
typedef std::unique_ptr<SceneObject_T> SceneObject;

// Global scene
class Scene {
private:
    VkQueue graphics_queue;
    uint32_t graphics_idx;

    VkCommandPool cmd_pool;
    VkCommandBuffer cmd;
    VkFence fence;

    MeshBuffers mesh_buffers;

    libgui::AutoDisposal disposal;

public:
    VmaAllocator VMA;
    VkDevice GPU;

    libgui::VkAllocatedImage DrawImage;
    libgui::VkAllocatedImage DrawDepth;

    PipelineLease PipelineLeaser;
    TextureLease TextureLeaser;
    libgui::DescriptorLease DescriptorLeaser;
    libgui::VkImmediateCommandBuffer ImmediateCmd;

    VkSampler DefaultLinearSampler;
    VkSampler DefaultNearestSampler;

    libgui::VkSizedBuffer PerDrawUniform;
    libgui::VkSizedBuffer SceneInfoUniform;

    VkDescriptorSetLayout UniversalSetLayout;
    VkDescriptorSet UniversalSet;

    std::vector<SceneObject> SceneObjects = {};

    explicit Scene(const vkb::Device &device, const TextureLease &texture_lease);

    void physics_tick();
    void frame_update();

    void poll_and_draw();
    void draw_once(const libgui::VkCompletePipeline &pipeline, const RenderDescription &desc);

    void ensure_uniform_size(size_t size);

    void dispose();

    Scene(Scene &&other) noexcept = default;

    Scene & operator=(Scene&& other) noexcept = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};

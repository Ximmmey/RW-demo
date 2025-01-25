#include "rendering.h"

class SceneCircle final : public SceneObject_T {
    std::shared_ptr<Scene> scene;
    glm::vec2 position;

    TexturePtr circle_image;
    VkDescriptorSetLayout texture_layout;
    VkDescriptorSet texture_set;
    LeasedPipeline pipeline;

public:
    explicit SceneCircle(const std::shared_ptr<Scene> &scene, const glm::vec2 pos) : scene(scene) {
        position = pos;
        if (!this->scene->TextureLeaser.try_get("circle32", &circle_image))
            circle_image = this->scene->TextureLeaser.load_file(this->scene->ImmediateCmd, "assets/circle.png", "circle32");

        texture_layout = {};
        VK_ASSERT( libgui::descriptor_set_layout(
            scene->GPU,
            {
                VkDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr),
            },

            &texture_layout
        ) );

        texture_set = scene->DescriptorLeaser.allocate(scene->TextureLeaser.GPU, texture_layout);
        libgui::DescriptorLayoutHelper()
            .image(0, circle_image->image.view, scene->DefaultNearestSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .update_set(scene->GPU, texture_set);

        // basic sprite pipeline
        VkShaderModule basic_vert;
        VkShaderModule basic_frag;

        libgui::vulkan_create_shader_from_file(scene->GPU, &basic_vert, "shaders/basic_sprite.vert.spv");
        libgui::vulkan_create_shader_from_file(scene->GPU, &basic_frag, "shaders/basic_sprite.frag.spv");

        pipeline = scene->PipelineLeaser.ensure_pipeline(
            scene->GPU,
            scene->DrawImage.format,
            { scene->UniversalSetLayout, texture_layout },
            { "shaders/basic_sprite.vert.spv", "shaders/basic_sprite.frag.spv" },
            { {basic_vert, VK_SHADER_STAGE_VERTEX_BIT}, {basic_frag, VK_SHADER_STAGE_FRAGMENT_BIT} }
        );

        vkDestroyShaderModule(scene->GPU, basic_vert, nullptr);
        vkDestroyShaderModule(scene->GPU, basic_frag, nullptr);
    }

    void physics_tick(Scene *scene) override {

    }

    void frame_update(Scene *scene) override {

    }

    void poll_draw() override {
        pipeline->poller.make_sprite(position, glm::vec2(32), -5, 1, scene->UniversalSet, texture_set);
    }
};

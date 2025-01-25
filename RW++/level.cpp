#pragma once

class SceneLevel final : public SceneObject_T {
    std::shared_ptr<Scene> scene;

    TexturePtr level_image;
    TexturePtr palette_image;
    TexturePtr noise_image;

    VkDescriptorSetLayout set_layout;
    VkDescriptorSet set;
    LeasedPipeline pipeline;

public:
    explicit SceneLevel(const std::shared_ptr<Scene> &scene, const char *level_asset) : scene(scene) {
        if (!scene->TextureLeaser.try_get(level_asset, &level_image))
            level_image = scene->TextureLeaser.load_file(scene->ImmediateCmd, level_asset, level_asset);

        if (!scene->TextureLeaser.try_get("palette0", &palette_image))
            palette_image = scene->TextureLeaser.load_file(scene->ImmediateCmd, "assets/palettes/palette0.png", "palette0");

        if (!scene->TextureLeaser.try_get("perlin64", &noise_image))
            noise_image = scene->TextureLeaser.load_file(scene->ImmediateCmd, "assets/noise.png", "perlin64");

        scene->ensure_uniform_size(sizeof(UniformLevelInfo));
        VK_ASSERT( libgui::descriptor_set_layout(
            scene->GPU,
            {
                VkDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
                VkDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
                VkDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
                VkDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
                VkDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            },

            &set_layout
        ) );

        set = scene->DescriptorLeaser.allocate(scene->TextureLeaser.GPU, set_layout);

        libgui::DescriptorLayoutHelper()
            .buffer(0, scene->PerDrawUniform.buffer, scene->PerDrawUniform.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .image(1, level_image->image.view, scene->DefaultNearestSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .image(2, palette_image->image.view, scene->DefaultNearestSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .image(3, noise_image->image.view, scene->DefaultNearestSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .image(4, scene->DrawDepth.view, scene->DefaultNearestSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .update_set(scene->GPU, set);

        // basic sprite pipeline
        VkShaderModule level_vert;
        VkShaderModule level_frag;

        VK_ASSERT( libgui::vulkan_create_shader_from_file(scene->GPU, &level_vert, "shaders/level.vert.spv") );
        VK_ASSERT( libgui::vulkan_create_shader_from_file(scene->GPU, &level_frag, "shaders/level.frag.spv") );

        pipeline = scene->PipelineLeaser.ensure_pipeline(
            scene->GPU,
            scene->DrawImage.format,
            { scene->UniversalSetLayout, set_layout },
            { "shaders/level.vert.spv", "shaders/level.frag.spv" },
            { {level_vert, VK_SHADER_STAGE_VERTEX_BIT}, {level_frag, VK_SHADER_STAGE_FRAGMENT_BIT} }
        );

        vkDestroyShaderModule(scene->GPU, level_vert, nullptr);
        vkDestroyShaderModule(scene->GPU, level_frag, nullptr);
    }

    void physics_tick(Scene *scene) override {

    }

    void frame_update(Scene *scene) override {

    }

    void poll_draw() override {
        constexpr glm::vec2 pos { 700, 400 };

        const auto level_info = std::make_shared<UniformLevelInfo>(
            glm::ivec2(1400, 800),
            0,
            0,
            glm::vec2(0, 0),

            glm::vec2(0, 0),

            glm::vec4(0, 0, 0, 0),
            0,
            0,
            1,
            0,
            0,
            0,
            1,
            1,
            1,
            0,
            0,

            0,

            glm::vec4(1),
            0
        );

        pipeline->poller.make_sprite(pos, glm::vec2(1400, 800), 0, 1, scene->UniversalSet, set, std::move(level_info));
    }
};

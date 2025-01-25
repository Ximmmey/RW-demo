#include "rendering.h"
#include "custom/bodychunk.h"
#include "custom/geometry.h"

class SimpleCollider final : public SceneObject_T {
    std::shared_ptr<Scene> scene;
    glm::vec2 camOffset;

    glm::vec2 gravity;

    TexturePtr circle_image;
    VkDescriptorSetLayout texture_layout;
    VkDescriptorSet texture_set;
    LeasedPipeline pipeline;

    BodyChunk bodychunk;

public:
    explicit SimpleCollider(const std::shared_ptr<Scene> &scene, const glm::vec2 pos, const glm::vec2 g, const glm::vec2 offset, RoomGeometry &room)
        : scene(scene),
          gravity(g),
          bodychunk(pos, 1.0f, 16.0f, 0.55f, 0.05f, room) {
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

        camOffset.x = -offset.x + 10.0f;
        camOffset.y = 800.0f + offset.y - (room.getYSize())*20.0f;

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
        bodychunk.Update(gravity);
    }

    void frame_update(Scene *scene) override {
        // CONTROLS
        if (ImGui::IsKeyDown(ImGuiKey_D)) bodychunk.addVelocity(glm::vec2(10, 0));
        if (ImGui::IsKeyDown(ImGuiKey_A)) bodychunk.addVelocity(glm::vec2(-10, 0));
        if (ImGui::IsKeyPressed(ImGuiKey_W)) bodychunk.addVelocity(glm::vec2(0, 50));

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            const auto mouse_pos = ImGui::GetMousePos();
            const auto set_pos_y = scene->DrawImage.height - mouse_pos.y;
            bodychunk.setPosition(glm::vec2(mouse_pos.x, set_pos_y) - camOffset);
        }

        // BODYCHUNK IMGUI
        bodychunk.draw_ui(camOffset, scene->DrawImage.height);

        // IMGUI
        ImGui::Begin("Player Control");

        // Draw line to the collider on-screen
        const auto dl = ImGui::GetBackgroundDrawList();
        const auto pos = bodychunk.getPosition() + camOffset;
        dl->AddLine(ImGui::GetWindowPos(), ImVec2(pos.x, scene->DrawImage.height - pos.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 2);

        ImGui::DragFloat("Gravity", &gravity.y, 50, -200, 200);

        ImGui::End();
    }

    void poll_draw() override {
        glm::vec2 onScreenPos = bodychunk.getPosition() + camOffset;
        pipeline->poller.make_sprite(onScreenPos, glm::vec2(32), -5, 1, scene->UniversalSet, texture_set);
    }
};

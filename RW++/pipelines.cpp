#include "pipelines.h"

LeasedPipeline_T::~LeasedPipeline_T() {
    owner_lease.Pipelines.erase(key);
    pipeline.dispose();

    wlog::log(wlog::WLOG_INFO, "deleted a pipeline!");
}

LeasedPipeline PipelineLease::ensure_pipeline(const VkDevice device, const VkFormat format, const std::vector<VkDescriptorSetLayout> &set_layouts, const std::vector<const char*> &shader_ids, const std::vector<std::tuple<VkShaderModule, VkShaderStageFlagBits>> &shaders) {
    const LeasePipelineInfo key { format, set_layouts, shader_ids };

    if (Pipelines.contains(key)) return Pipelines.at(key).lock();

    auto builder = libgui::PipelineBuilder()
        .color_attachment_format(format)
        .multisampling()
        .color_blending_alpha()

        .topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .polygon_mode(VK_POLYGON_MODE_FILL)
        .cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)

        .depth_format(VK_FORMAT_D16_UNORM)
        .depth_test_default(VK_COMPARE_OP_LESS_OR_EQUAL)

        .push_vertex_field(0, VK_FORMAT_R32G32B32_SFLOAT, 0) // vec3 pos
        .push_vertex_field(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)) // vec2 uv
        .push_vertex_field(2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)) // vec4 color
        .push_vertex_binding<Vertex>();

    for (const auto layout: set_layouts) {
        builder.push_layout(layout);
    }

    for (auto [shader, stage]: shaders) {
        builder.push_shader(shader, stage);
    }

    auto pipeline = std::make_shared<LeasedPipeline_T>(*this, key, builder.build(device));
    Pipelines.emplace(key, std::weak_ptr(pipeline));

    return pipeline;
}

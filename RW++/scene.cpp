#include "rendering.h"
#include "scene.h"
#include "uniforms.h"
#include "glm_fix.h"

Scene::Scene(const vkb::Device &device, const TextureLease &texture_lease) : VMA(texture_lease.VMA), GPU(device), TextureLeaser(texture_lease) {
    graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
    graphics_idx = device.get_queue_index(vkb::QueueType::graphics).value();

    disposal = libgui::AutoDisposal();

    DrawImage = {};
    libgui::create_image(VMA, device, &DrawImage, 1400, 800, 0, VK_FORMAT_R8G8B8A8_UNORM);

    DrawDepth = {};
    libgui::create_image(VMA, device, &DrawDepth, 1400, 800, 0, VK_FORMAT_D16_UNORM, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    disposal.push_back([&] {
        DrawImage.dispose();
        DrawDepth.dispose();
    });

    // pipeline leaser
    PipelineLeaser = {};

    // Render fence
    fence = VK_NULL_HANDLE;
    constexpr auto fence_create = VkFenceCreateInfo(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

    VK_ASSERT(vkCreateFence(GPU, &fence_create, nullptr, &fence));
    disposal.push_back([&] {
        vkDestroyFence(GPU, fence, nullptr);
    });

    // Command Pool
    cmd_pool = VK_NULL_HANDLE;
    const VkCommandPoolCreateInfo pool_create {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_idx,
    };

    VK_ASSERT(vkCreateCommandPool(GPU, &pool_create, nullptr, &cmd_pool));

    // Command buffer
    cmd = VK_NULL_HANDLE;
    const VkCommandBufferAllocateInfo command_buffer_create {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_ASSERT(vkAllocateCommandBuffers(GPU, &command_buffer_create, &cmd));

    disposal.push_back([&] {
        vkDestroyCommandPool(GPU, cmd_pool, nullptr);
    });

    mesh_buffers = {};

    VK_ASSERT( libgui::create_buffer(VMA, &mesh_buffers.vertices, 64, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) );

    VK_ASSERT( libgui::create_buffer(VMA, &mesh_buffers.indices, 64, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) );

    disposal.push_back([&] {
        mesh_buffers.vertices.dispose();
        mesh_buffers.indices.dispose();
    });

    // immediate command buffer
    ImmediateCmd = libgui::VkImmediateCommandBuffer(device);
    disposal.push_back([&] {
        ImmediateCmd.dispose();
    });

    // Samplers
    constexpr VkSamplerCreateInfo linear_sampler_create {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
    };
    vkCreateSampler(device, &linear_sampler_create, nullptr, &DefaultLinearSampler);

    constexpr VkSamplerCreateInfo nearest_sampler_create {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
    };
    vkCreateSampler(device, &nearest_sampler_create, nullptr, &DefaultNearestSampler);

    disposal.push_back([&] {
        vkDestroySampler(GPU, DefaultLinearSampler, nullptr);
        vkDestroySampler(GPU, DefaultNearestSampler, nullptr);
    });

    // Descriptor lease
    std::vector<libgui::DescriptorLease::PoolSizeRatio> desc_lease_sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
    };

    DescriptorLeaser.init(GPU, 512, desc_lease_sizes);
    disposal.push_back([&] {
        DescriptorLeaser.destroy_pools(GPU);
    });

    PerDrawUniform = {};
    VK_ASSERT( libgui::create_buffer(VMA, &PerDrawUniform, 64, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) );

    SceneInfoUniform = {};
    VK_ASSERT( libgui::create_buffer(VMA, &SceneInfoUniform, 64, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) );

    disposal.push_back([&] {
        SceneInfoUniform.dispose();
    });

    VK_ASSERT( libgui::descriptor_set_layout(
        device,
        {
            VkDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr),
        },
        &UniversalSetLayout
    ) );
    disposal.push_back([&] { vkDestroyDescriptorSetLayout(GPU, UniversalSetLayout, nullptr); });
    UniversalSet = DescriptorLeaser.allocate(device, UniversalSetLayout);

    libgui::DescriptorLayoutHelper()
        .buffer(0, SceneInfoUniform.buffer, SceneInfoUniform.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        .update_set(device, UniversalSet);
}

void Scene::frame_update() {
    for (const auto &obj: SceneObjects) {
        obj->frame_update(this);
    }
}

void Scene::physics_tick() {
    for (const auto &obj: SceneObjects) {
        obj->physics_tick(this);
    }
}

void Scene::poll_and_draw() {
    vkResetFences(GPU, 1, &fence);
    VK_ASSERT(vkResetCommandBuffer(cmd, 0));

    const VkCommandBufferBeginInfo cmdBeginInfo = libgui::command_buffer_begin_info();
    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    const UniformSceneInfo screen_mat {
        .transform = ortho(0, DrawImage.width, DrawImage.height, 0, 0, 30), // X+ right, Y+ up, Z+ away
    };
    vkCmdUpdateBuffer(cmd, SceneInfoUniform.buffer, VkDeviceSize {0}, sizeof(UniformSceneInfo), &screen_mat);

    // wait for update to finish
    libgui::uniform_write_barrier(cmd, SceneInfoUniform, 0);

    // draw and depth images UNDEF -> GENERAL
    libgui::change_image_layout(cmd, DrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    libgui::change_image_layout(cmd, DrawDepth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Clear draw image
    constexpr VkClearColorValue clear = { {0.01f, 0.01f, 0.01f, 1.0f} };
    const auto clear_range = libgui::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, DrawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &clear_range);

    // Clear depth image
    constexpr VkClearDepthStencilValue clear_depth = { 1 };
    const auto clear_range_depth = libgui::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT);
    vkCmdClearDepthStencilImage(cmd, DrawDepth.image, VK_IMAGE_LAYOUT_GENERAL, &clear_depth, 1, &clear_range_depth);

    // draw and depth images GENERAL -> ATTACHMENT
    libgui::change_image_layout(cmd, DrawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    libgui::change_image_layout(cmd, DrawDepth.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    const VkViewport render_viewport { 0, 0, static_cast<float>(DrawImage.width), static_cast<float>(DrawImage.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &render_viewport);

    const VkRect2D render_scissor { 0, 0, DrawImage.width, DrawImage.height };
    vkCmdSetScissor(cmd, 0, 1, &render_scissor);

    // Draw all pipelines
    for (const auto &pipeline: PipelineLeaser.Pipelines | std::views::values) {
        const auto locked = pipeline.lock();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, locked->pipeline.pipeline);

        for (const auto &desc: locked->poller.Descriptions) {
            draw_once(locked->pipeline, desc);
        }
    }

    vkEndCommandBuffer(cmd);

    const VkCommandBufferSubmitInfo cmd_info = libgui::command_buffer_submit_info(cmd);
    const VkSubmitInfo2 submit = libgui::submit_info(&cmd_info, nullptr, nullptr);

    VK_ASSERT(vkQueueSubmit2(graphics_queue, 1, &submit, fence));

    // Reset all pollers and poll all objects for next frame
    PipelineLeaser.reset_pollers();
    for (const auto &obj : SceneObjects) {
        obj->poll_draw();
    }

    VK_ASSERT(vkWaitForFences(GPU, 1, &fence, true, 9999999999)); // :trolley:
}

void Scene::draw_once(const libgui::VkCompletePipeline &pipeline, const RenderDescription &desc) {
    const uint32_t size_of_vertices = sizeof(Vertex) * desc.mesh_vertices.size();
    const uint32_t size_of_indices = sizeof(uint16_t) * desc.mesh_indices.size();

    // Resize buffers if needed
    if (mesh_buffers.vertices.size < size_of_vertices) {
        mesh_buffers.vertices.dispose();
        VK_ASSERT( libgui::create_buffer(VMA, &mesh_buffers.vertices, size_of_vertices, VMA_MEMORY_USAGE_GPU_ONLY, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) );

        wlog::logf(wlog::WLOG_INFO, "RESIZED VERTEX BUFFER: %d", mesh_buffers.vertices.size);
    }

    if (mesh_buffers.indices.size < size_of_indices) {
        mesh_buffers.indices.dispose();
        VK_ASSERT( libgui::create_buffer(VMA, &mesh_buffers.indices, size_of_indices, VMA_MEMORY_USAGE_GPU_ONLY, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) );

        wlog::logf(wlog::WLOG_INFO, "RESIZED INDEX BUFFER: %d", mesh_buffers.indices.size);
    }

    // SHARED UNIFORM BUFFER IS UPDATED STATICALLY,
    // SEE: Scene::ensure_uniform_size

    // Wait till reads are done and we can write
    libgui::vertex_read_barrier(cmd, mesh_buffers.vertices, 0);
    libgui::index_read_barrier(cmd, mesh_buffers.indices, 0);
    libgui::uniform_read_barrier(cmd, PerDrawUniform, 0);

    // Update buffers
    constexpr VkDeviceSize upload_offset = 0;
    vkCmdUpdateBuffer(cmd, mesh_buffers.vertices.buffer, upload_offset, size_of_vertices, desc.mesh_vertices.data());
    vkCmdUpdateBuffer(cmd, mesh_buffers.indices.buffer, upload_offset, size_of_indices, desc.mesh_indices.data());
    if (desc.uniform_size > 0) vkCmdUpdateBuffer(cmd, PerDrawUniform.buffer, upload_offset, desc.uniform_size, desc.uniform.get());

    // Ensure write is done and coherent
    libgui::vertex_write_barrier(cmd, mesh_buffers.vertices, 0);
    libgui::index_write_barrier(cmd, mesh_buffers.indices, 0);
    libgui::uniform_write_barrier(cmd, PerDrawUniform, 0);

    // Start rendering
    const VkRenderingAttachmentInfo color_attachment = libgui::attachment_info(DrawImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingAttachmentInfo depth_attachment = libgui::attachment_info(DrawDepth.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo renderInfo = libgui::rendering_info(VkRect2D { 0, 0, DrawImage.width, DrawImage.height }, &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    const VkDescriptorSet sets[2] = { desc.scene_set, desc.object_set };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 2, sets, 0, nullptr);

    // Bind vtx, idx and then draw
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh_buffers.vertices.buffer, &upload_offset);
    vkCmdBindIndexBuffer(cmd, mesh_buffers.indices.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, desc.mesh_indices.size(), 1, 0, 0, 0);

    vkCmdEndRendering(cmd);
}

void Scene::ensure_uniform_size(const size_t size) {
    if (PerDrawUniform.size < size) {
        PerDrawUniform.dispose();
        VK_ASSERT( libgui::create_buffer(VMA, &PerDrawUniform, size, VMA_MEMORY_USAGE_GPU_ONLY, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) );

        wlog::logf(wlog::WLOG_INFO, "RESIZED UNIFORM BUFFERS: %d", size);
    }
}


void Scene::dispose() {
    PerDrawUniform.dispose();
    disposal.dispose();
}

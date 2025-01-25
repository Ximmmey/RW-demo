#pragma once

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>

namespace libgui {

    /**
     * @brief initialises imgui for libgui
     * @param gui_manager the main GUI Manager
     * @return string variant. None if init was successful
     */
static std::optional<std::string> imgui_init(GUIManager &gui_manager) {
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    auto io = ImGui::GetIO();
    ImGui_ImplSDL3_InitForVulkan(gui_manager.Window);

    VkFormat format[] = { VK_FORMAT_R8G8B8A8_UNORM };
    const VkPipelineRenderingCreateInfo dynamic_create {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,

        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = format,

        .depthAttachmentFormat = VK_FORMAT_D16_UNORM,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    ImGui_ImplVulkan_InitInfo imgui_vk_info {
        .Instance = gui_manager.Vulkan,

        .PhysicalDevice = gui_manager.PhysicalGPU,
        .Device = gui_manager.GPU,

        .QueueFamily = gui_manager.GraphicsQueueIdx,
        .Queue = gui_manager.GraphicsQueue,

        .DescriptorPool = VK_NULL_HANDLE,

        .MinImageCount = 2,
        .ImageCount = gui_manager.Swapchain.image_count,

        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,

        .PipelineCache = VK_NULL_HANDLE,

        .DescriptorPoolSize = 32,

        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = dynamic_create,

        .Allocator = nullptr,
        .MinAllocationSize = 1024 * 1024,
    };

    ImGui_ImplVulkan_Init(&imgui_vk_info);

    gui_manager.Disposal.push_back([&] {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(ImGui::GetCurrentContext());
    });

    return {};
}

/**
 * @brief Call this before imgui calls
 */
static void imgui_frame_begin() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

/**
 * @brief Call this after all imgui calls have been done
 */
static void imgui_frame_end() {
    ImGui::EndFrame();

    if (const ImGuiIO &io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

/**
 * @brief Hook this into the end of @code GUIManager::sync_draw_frame(fn) @endcode
 * @param cmd command buffer
 * @param draw image to draw to
 */
static void imgui_draw(const VkCommandBuffer cmd, const VkAllocatedImage &draw) {
    ImGui::Render();

    const VkRenderingAttachmentInfo color_attachment = libgui::attachment_info(draw.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo rendering_info = libgui::rendering_info(VkRect2D {0, 0, draw.extent.width, draw.extent.height}, &color_attachment);

    vkCmdBeginRendering(cmd, &rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

}

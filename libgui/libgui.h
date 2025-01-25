#pragma once

#ifdef _WIN32
    #pragma comment(linker, "/subsystem:windows")
    #define VK_USE_PLATFORM_WIN32_KHR
    #define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#include "libgui_pipeline.h"
#include "libgui_init.h"
#include "libgui_barriers.h"
#include "libgui_utils.h"
#include "libgui_vkutils.h"
#include "libgui_vma.h"
#include "wlog.h"

#include <volk.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <cmath>
#include <memory>
#include <thread>
#include <vector>

/**
 * Library for managing SDL, Vulkan and Imgui easily.
 *
 * DISPLAY TEXTURES PREFER RGBA8 UNORM
 */
namespace libgui {

class GUIManager {
    bool frame_begun = false;

public:
    bool WindowOpen = false;
    SDL_Window *Window = nullptr;
    uint32_t WindowWidth = 1400;
    uint32_t WindowHeight = 800;

    AutoDisposal Disposal = {};

    vkb::Instance Vulkan;
    std::optional<VkDebugUtilsMessengerEXT> VulkanLog;
    vkb::PhysicalDevice PhysicalGPU;
    vkb::Device GPU;

    VmaAllocator VMA = VMA_NULL;

    VkQueue GraphicsQueue = VK_NULL_HANDLE;
    uint32_t GraphicsQueueIdx = 0;

    VkQueue PresentQueue = VK_NULL_HANDLE;
    uint32_t PresentQueueIdx = 0;

    bool resize_requested = false;
    bool presenting = true;
    VkSurfaceKHR Surface = VK_NULL_HANDLE;
    vkb::Swapchain Swapchain;
    VkAllocatedImage DrawImage;

    uint32_t FrameCount = 0;
    VkFrameData Frames[2] = { };

    //////////////////////////////////////////////////////////////

    DescriptorLease GlobalDescriptorAllocator;

    /**
     * @brief Initialises SDL
     * @return Variant string of error, no value if no error
     * @param title Title of the SDL window
     */
    std::optional<std::string> sdl_init(const char* title) {
        log(wlog::WLOG_TRACE, "Initialise SDL and Window");
        if (!SDL_Init(SDL_INIT_VIDEO))
            return std::string("Failed to init SDL for GUIManager: ") + SDL_GetError();

        Window = SDL_CreateWindow(title, WindowWidth, WindowHeight, SDL_WINDOW_VULKAN /*| SDL_WINDOW_RESIZABLE*/);

        WindowOpen = true;

        return {};
    }

    /**
     * @brief Polls SDL events and does minimal window controls.\n
     * Use the hook argument to inject your own state switching.
     * @param hook function you want to call before the internal switch cases
     */
    void event(const std::function<void(SDL_Event)> &hook) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            hook(event);

            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    WindowOpen = false;
                    break;
                }

                case SDL_EVENT_WINDOW_MINIMIZED: {
                    presenting = false;
                    break;
                }

                case SDL_EVENT_WINDOW_RESTORED: {
                    presenting = true;
                    break;
                }

                case SDL_EVENT_WINDOW_MAXIMIZED:
                case SDL_EVENT_WINDOW_RESIZED: {
                    sdl_window_size(Window, &WindowWidth, &WindowHeight);
                    break;
                }

                default: ;
            }
        }

        // Recreate swapchain before drawing
        if (resize_requested) {
            recreate_swapchain();
            resize_requested = false;
        }
    }

    /**
     * @brief Initialises vulkan and internal state for rendering and presentation
     * @return Variant string of error, no value if no error
     * @param appName Name of the program
     * @param appVersion Version of the program. Use VK_MAKE_VERSION()
     * @param engineName Name of the engine (if exists, prefer "NONE" if no engine)
     * @param engineVersion Version of the engine. Use VK_MAKE_VERSION()
     * @param useVVL Whether to use the Vulkan Validation Layers
     * @param allowedVulkanLogs Allowed vulkan logs to pass to wlog. Separate from wlog disables
     */
    std::optional<std::string> vulkan_init(
        const char *appName,
        const uint32_t appVersion,
        const char *engineName,
        const uint32_t engineVersion,
        const bool useVVL,
        const VkDebugUtilsMessageSeverityFlagsEXT allowedVulkanLogs
    ) {
        vkb::InstanceBuilder instance_builder {};
        instance_builder
            .require_api_version(1, 3, 0)

            .set_app_name(appName)
            .set_app_version(appVersion)
            .set_engine_name(engineName)
            .set_engine_version(engineVersion)

            .request_validation_layers(useVVL)
            .enable_validation_layers()
            .add_debug_messenger_severity(allowedVulkanLogs)
            .set_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            .set_debug_callback(vulkan_debug_callback);

        const auto instance_result = instance_builder.build();

        if (!instance_result)
            return "Failed to create vk instance: " + instance_result.error().message();

        Vulkan = instance_result.value();

        volkLoadInstanceOnly(Vulkan);
        Disposal.push_back([&] { vkb::destroy_instance(Vulkan); });

        // Create SDL KHR Surface
        if (!SDL_Vulkan_CreateSurface(Window, Vulkan, nullptr, &Surface))
            return "Failed to create Vulkan surface with SDL: " + std::string(SDL_GetError());

        Disposal.push_back([&] {
            vkDestroySurfaceKHR(Vulkan, Surface, nullptr);
        });

        // ============== EXTENSIONS YOU NEED ==============

        // =================================================

        // DEVICES
        vkb::PhysicalDeviceSelector phys_device_selector(Vulkan);
        auto physical_device_selector_return = phys_device_selector
            .set_minimum_version(1, 3)
            .set_required_features_12(VkPhysicalDeviceVulkan12Features {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,

                .descriptorIndexing = true,
                .bufferDeviceAddress = true,
            })
            .set_required_features_13(VkPhysicalDeviceVulkan13Features {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,

                .synchronization2 = true,
                .dynamicRendering = true,
            })

        // ============== EXTENSIONS YOU NEED ==============

        // =================================================

            .set_surface(Surface)
            .select();

        if (!physical_device_selector_return)
            return "Failed to fetch physical GPU: " + physical_device_selector_return.error().message();

        PhysicalGPU = physical_device_selector_return.value();
        // PhysicalGPU.enable_extensions_if_present();

        // LOGICAL DEVICE
        vkb::DeviceBuilder device_builder(PhysicalGPU);
        auto device_return = device_builder.build();

        if (!device_return.has_value())
            return "Failed to create logical GPU: " + device_return.error().message();

        GPU = device_return.value();

        GraphicsQueue = GPU.get_queue(vkb::QueueType::graphics).value();
        GraphicsQueueIdx = GPU.get_queue_index(vkb::QueueType::graphics).value();

        PresentQueue = GPU.get_queue(vkb::QueueType::present).value();
        PresentQueueIdx = GPU.get_queue_index(vkb::QueueType::present).value();

        volkLoadDevice(GPU);
        Disposal.push_back([&] { vkb::destroy_device(GPU); });

        // VMA ALLOCATOR
        VMA = vma_init(Vulkan, PhysicalGPU, GPU);
        Disposal.push_back([&] { vmaDestroyAllocator(VMA); });

        // SWAPCHAIN
        if (const auto swapchain_result = recreate_swapchain(); swapchain_result.has_value())
            return "Failed to create swapchain: " + swapchain_result.value();

        // Swapchain is destroyed and recreated very commonly

        // FRAMES
        for (auto &frame : Frames) {
            if (auto frame_res = frame.init(GPU); frame_res != VK_SUCCESS)
                return "FAILED TO INITIALISE A FRAME DATA: " + std::to_string(frame_res);
        }

        Disposal.push_back([&] {
            for (const auto &frame : Frames) {
                vkDestroyCommandPool(GPU, frame.cmd_pool, nullptr);

                vkDestroyFence(GPU, frame.present_fence, nullptr);
                vkDestroySemaphore(GPU, frame.signal_semaphore, nullptr);
                vkDestroySemaphore(GPU, frame.wait_semaphore, nullptr);
            }
        });

        return {};
    }

    /**
     * @brief Recreates the present swapchain.\n
     * INTENDED FOR INTERNAL USE.
     * @return string variant of error. No error if it was successful
     */
    std::optional<std::string> recreate_swapchain() {
        if (WindowWidth * WindowHeight == 0) return {};
        vkDeviceWaitIdle(GPU);

        auto recreate_swp = vkb::SwapchainBuilder(PhysicalGPU, GPU, Surface)
            .set_desired_format(VkSurfaceFormatKHR { .format = VK_FORMAT_R8G8B8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .add_fallback_format(VkSurfaceFormatKHR { .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(WindowWidth, WindowHeight)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build();

        if (!recreate_swp)
            return recreate_swp.error().message() + " " + std::to_string(recreate_swp.vk_result());

        vkb::destroy_swapchain(Swapchain);
        if (DrawImage.view != VK_NULL_HANDLE) DrawImage.dispose();

        Swapchain = recreate_swp.value();
        create_image(VMA, GPU, &DrawImage, WindowWidth, WindowHeight, 0, Swapchain.image_format);

        return {};
    }

    /**
     * @brief Fetches the frame to present this frame
     * @return The current present frame
     */
    VkFrameData & c_frame() { return Frames[FrameCount % 2]; };

    /**
     * @brief Syncs and draws to the screen.\n
     * Hook the function arg for the draws you want to make
     * @param draws The draws you want to call every frame
     */
    void sync_draw_frame(const std::function<void (VkCommandBuffer, VkAllocatedImage)> &draws) {
        if (!presenting) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return;
        };

        VkFrameData frame = c_frame();

        // Wait for next frame
        VK_ASSERT( vkWaitForFences(GPU, 1, &frame.present_fence, true, 1000000000) );

        VK_ASSERT( vkResetFences(GPU, 1, &frame.present_fence) );

        frame.per_frame_disposal.dispose();

        uint32_t swap_idx;
        VkResult swp_result = vkAcquireNextImageKHR(GPU, Swapchain, 1000000000, frame.wait_semaphore, nullptr, &swap_idx);

        // swapchain is being meddled with! (Resize or minimised)
        if (swp_result == VK_ERROR_OUT_OF_DATE_KHR) {
            resize_requested = true;
            return;
        }

        const VkImage swap_img = Swapchain.get_images().value()[swap_idx];

        const VkCommandBuffer cmd = frame.cmd_buffer;
        VK_ASSERT(vkResetCommandBuffer(cmd, 0));

        const auto begin = command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_ASSERT(vkBeginCommandBuffer(cmd, &begin));

        change_image_layout(cmd, DrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // Clear Screen
        constexpr VkClearColorValue clear = { {0.01f, 0.01f, 0.01f, 1.0f} };
        const auto clear_range = image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, DrawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &clear_range);

        draws(cmd, DrawImage);

        // DRAW GENERAL -> SRC
        change_image_layout(cmd, DrawImage.image,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        // NEXT FRAME UNDEF -> DST
        change_image_layout(cmd, swap_img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy from our main draw image to next frame image
        blit_image(cmd, DrawImage.image, VkExtent2D(DrawImage.width, DrawImage.height), swap_img, VkExtent2D(WindowWidth, WindowHeight));

        // We're ready to present now
        change_image_layout(cmd, swap_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_ASSERT( vkEndCommandBuffer(cmd) );

        const VkCommandBufferSubmitInfo submit = command_buffer_submit_info(cmd);
        const VkSemaphoreSubmitInfo wait = semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.wait_semaphore);
        const VkSemaphoreSubmitInfo signal = semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.signal_semaphore);

        const VkSubmitInfo2 submit_inf = submit_info(&submit, &signal, &wait);

        VK_ASSERT(vkQueueSubmit2(GraphicsQueue, 1, &submit_inf, frame.present_fence))

        // Display on the window
        const auto present = present_info(&Swapchain.swapchain, &swap_idx, &frame.signal_semaphore);
        if (vkQueuePresentKHR(GraphicsQueue, &present) == VK_ERROR_OUT_OF_DATE_KHR) {
            resize_requested = true;
        }

        FrameCount++;
    }

    /**
     * @brief Disposes of all the internal allocations and calls the main auto disposal
     */
    void dispose() {
        vkDeviceWaitIdle(GPU);

        vkb::destroy_swapchain(Swapchain);
        DrawImage.dispose();
        Disposal.dispose();
    }
};

}

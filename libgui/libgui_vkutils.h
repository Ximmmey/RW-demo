#pragma once

#include "libgui_init.h"
#include "libgui_vma.h"
#include "wlog.h"

#include <VkBootstrap.h>
#include <volk.h>

#include <SDL3/SDL.h>

#ifndef WIN32
#include <bits/fs_fwd.h>
#include <bits/fs_path.h>
#else
#include <filesystem>
#endif
#include <deque>
#include <functional>
#include <ranges>
#include <vector>

#define VK_RESULT_EARLY_RETURN(RESULT) {                                      \
    VkResult result = RESULT;                                                 \
    if (result < VK_SUCCESS)                                                  \
    {                                                                         \
        return __FILE_NAME__ + std::to_string(__LINE__) + "FAILED AUTO CHECK" \
    }                                                                         \
}

#define VK_ASSERT(RESULT) { VkResult result = RESULT; assert(result >= VK_SUCCESS); }

namespace libgui {

/**
 * Poor man's GC.
 */
struct AutoDisposal {
    std::deque<std::function<void()>> funcs;

    void push_back(std::function<void()> &&function) {
        funcs.push_back(function);
    }

    void dispose() {
        for (auto & func : std::ranges::reverse_view(funcs)) {
            func(); //call functors
        }

        funcs.clear();
    }
};

/**
 * @brief A command buffer to be used for "immediate" calls
 */
struct VkImmediateCommandBuffer {
    VkDevice gpu = VK_NULL_HANDLE;

    VkQueue graphics_queue = VK_NULL_HANDLE;
    uint32_t graphics_queue_idx = 0;

    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    VkFence fence = VK_NULL_HANDLE;

    VkImmediateCommandBuffer() = default;

    explicit VkImmediateCommandBuffer(const vkb::Device &device) : gpu(device) {
        graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
        graphics_queue_idx = device.get_queue_index(vkb::QueueType::graphics).value();

        fence = VK_NULL_HANDLE;
        constexpr auto fence_create = VkFenceCreateInfo(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

        VK_ASSERT(vkCreateFence(gpu, &fence_create, nullptr, &fence));

        // Command Pool
        pool = VK_NULL_HANDLE;
        const VkCommandPoolCreateInfo pool_create {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics_queue_idx,
        };

        VK_ASSERT(vkCreateCommandPool(gpu, &pool_create, nullptr, &pool));

        // Command buffer
        cmd = VK_NULL_HANDLE;
        const VkCommandBufferAllocateInfo command_buffer_create {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VK_ASSERT(vkAllocateCommandBuffers(gpu, &command_buffer_create, &cmd));
    }

    void dispose() const {
        vkQueueWaitIdle(graphics_queue);

        vkDestroyFence(gpu, fence, nullptr);

        vkResetCommandBuffer(cmd, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        vkDestroyCommandPool(gpu, pool, nullptr);
    }
};

/**
 * @brief Per-frame data for presentation buffering
 */
struct VkFrameData {
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;

    VkFence present_fence = VK_NULL_HANDLE;
    VkSemaphore wait_semaphore = VK_NULL_HANDLE, signal_semaphore = VK_NULL_HANDLE;

    AutoDisposal per_frame_disposal;

    VkResult init(const vkb::Device &GPU) {
        const VkCommandPoolCreateInfo pool_create {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = GPU.get_queue_index(vkb::QueueType::graphics).value(),
        };

        VK_ASSERT(vkCreateCommandPool(GPU, &pool_create, nullptr, &cmd_pool));

        // allocate the default command buffer that we will use for rendering
        const VkCommandBufferAllocateInfo command_buffer_create{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = cmd_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
        };

        constexpr VkFenceCreateInfo fence_create = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        VK_ASSERT(vkAllocateCommandBuffers(GPU, &command_buffer_create, &cmd_buffer));

        VK_ASSERT(vkCreateFence(GPU, &fence_create, nullptr, &present_fence));

        constexpr VkSemaphoreCreateInfo semaphore_create = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VK_ASSERT(vkCreateSemaphore(GPU, &semaphore_create, nullptr, &wait_semaphore));

        VK_ASSERT(vkCreateSemaphore(GPU, &semaphore_create, nullptr, &signal_semaphore));

        return VK_SUCCESS;
    }
};

/**
 * @brief a VMA allocated image
 */
struct VkAllocatedImage {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VmaAllocator allocator;
    VkExtent3D extent;
    VkFormat format;

    uint32_t width;
    uint32_t height;

    VkAllocatedImage() = default;

    void dispose() const {
        vkDestroyImageView(allocator->m_hDevice, view, nullptr);
        vmaDestroyImage(allocator, image, allocation);
    }
};

/**
 * @brief Creates a VMA allocated image
 */
inline void create_image(
    const VmaAllocator vma,
    const VkDevice device,
    VkAllocatedImage *image,
    const uint32_t width,
    const uint32_t height,
    const VmaAllocationCreateFlags alloc_flags,
    const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT,
    const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
    const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    const VkImageCreateFlags flags = 0
) {
    image->allocator = vma;
    image->width = width;
    image->height = height;

    image->format = format;
    image->extent = VkExtent3D {
        width,
        height,
        1
    };

    const auto img_create = image_create_info(format, usage, image->extent, flags);

    const VmaAllocationCreateInfo alloc_info = {
        .flags = alloc_flags,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VK_ASSERT( vmaCreateImage(vma, &img_create, &alloc_info, &image->image, &image->allocation, nullptr) );

    const VkImageViewCreateInfo view_create = imageview_create_info(format, image->image, aspect);
    VK_ASSERT( vkCreateImageView(device, &view_create, nullptr, &image->view) );
}

static VkResult vulkan_create_shader_from_file(const VkDevice logi_device, VkShaderModule *shader, const char *path) {
    std::ifstream file(path, std::ios::binary);

    const auto size = std::filesystem::file_size(path);
    std::vector<uint32_t> spv(size);
    file.read(reinterpret_cast<char*>(spv.data()), size);

    const VkShaderModuleCreateInfo create {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = spv.data(),
    };

    return vkCreateShaderModule(logi_device, &create, nullptr, shader);
}

/**
 * @brief Immediately records, queues and waits for the given calls
 * @param cmd Immediate Command Buffer to record to
 * @param call Calls to record
 */
static void cmd_immediate(const VkImmediateCommandBuffer &cmd, const std::function<void()> &call) {
    VK_ASSERT(vkResetFences(cmd.gpu, 1, &cmd.fence));
    VK_ASSERT(vkResetCommandBuffer(cmd.cmd, 0));

    const VkCommandBufferBeginInfo begin_info = command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_ASSERT(vkBeginCommandBuffer(cmd.cmd, &begin_info));

    call();

    VK_ASSERT(vkEndCommandBuffer(cmd.cmd));

    const VkCommandBufferSubmitInfo cmd_info = command_buffer_submit_info(cmd.cmd);
    const VkSubmitInfo2 submit = submit_info(&cmd_info, nullptr, nullptr);

    VK_ASSERT(vkQueueSubmit2(cmd.graphics_queue, 1, &submit, cmd.fence));

    VK_ASSERT(vkWaitForFences(cmd.gpu, 1, &cmd.fence, true, 9999999999));
}

/**
 * @brief Copies a source image to a given image
 * @param cmd Command buffer
 * @param src source image
 * @param src_region where to copy
 * @param dst Destination image
 * @param dst_region Where to copy to
 */
static void blit_image(const VkCommandBuffer cmd, const VkImage src, const VkExtent2D src_region, const VkImage dst, const VkExtent2D dst_region) {
    VkImageBlit2 region {
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,

        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },

        .srcOffsets = {
            {}, { static_cast<int32_t>(src_region.width), static_cast<int32_t>(src_region.height), 1 }
        },

        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },

        .dstOffsets = {
            {}, { static_cast<int32_t>(dst_region.width), static_cast<int32_t>(dst_region.height), 1 }
        },
    };

    const VkBlitImageInfo2 info {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,

        .srcImage = src,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

        .dstImage = dst,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

        .regionCount = 1,
        .pRegions = &region,

        .filter = VK_FILTER_NEAREST,
    };

    vkCmdBlitImage2(cmd, &info);
}

/**
 * @brief Copies contents of a buffer to another buffer
 * @param cmd Command buffer
 * @param src_buffer Buffer to copy
 * @param src_offset Offset of copy
 * @param dst_buffer Buffer to copy to
 * @param dst_offset Offset of destination
 * @param size Size of data to copy
 */
static void buffer_to_buffer(const VkCommandBuffer cmd, const VkBuffer &src_buffer, const uint32_t src_offset, const VkBuffer &dst_buffer, const uint32_t dst_offset, const uint32_t size) {
    const VkBufferCopy2 region {
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = size,
    };

    const VkCopyBufferInfo2 copy {
        .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
        .srcBuffer = src_buffer,
        .dstBuffer = dst_buffer,
        .regionCount = 1,
        .pRegions = &region
    };

    vkCmdCopyBuffer2(cmd, &copy);
}

/**
 * @brief creates a staging buffer and uses it to copy given data to a destination buffer
 * @attention Don't forget to dispose of the created staging buffer!
 * @param cmd Command buffer
 * @param upload Pointer to an uninitialised sized buffer
 * @param vma The VMA allocator
 * @param data Data to copy
 * @param size Size of data
 * @param src_offset Offset of data
 * @param dst_buffer Buffer to copy to
 * @param dst_offset Buffer offset
 */
static void data_to_buffer(const VkCommandBuffer cmd, VkSizedBuffer &upload, const VmaAllocator vma, const void *data, const uint32_t size, const uint32_t src_offset, const VkBuffer &dst_buffer, const uint32_t dst_offset) {
    VK_ASSERT( create_buffer(vma, &upload, size, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT) );
    memcpy(upload.allocation_info.pMappedData, data, size);

    buffer_to_buffer(cmd, upload.buffer, src_offset, dst_buffer, dst_offset, size);
}

static void data_to_image(const VkCommandBuffer cmd, VkSizedBuffer &upload, const VmaAllocator vma, const void *data, const uint32_t size, const uint32_t src_offset, const VkImage dst, const uint32_t dst_w, const uint32_t dst_h) {
    VK_ASSERT( create_buffer(vma, &upload, size, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT) );
    memcpy(upload.allocation_info.pMappedData, data, size);

    const VkBufferImageCopy region = {
        .bufferOffset = src_offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = VkOffset3D(0, 0, 0),
        .imageExtent = VkExtent3D(dst_w, dst_h, 1),
    };

    vkCmdCopyBufferToImage(cmd, upload.buffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

/**
 * @brief util for SDL window size.\n
 * Same as SDL_GetWindowSizeInPixels but uses uint32_t
 */
inline void sdl_window_size(SDL_Window *window, uint32_t *width, uint32_t *height) {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    *width = w;
    *height = h;
}

/**
 * @brief Converts vulkan log severity to wlog severity
 */
constexpr wlog::wlogSeverity vulkan_log_bit_to_wlog(const VkDebugUtilsMessageSeverityFlagBitsEXT vk_severity) {
    switch (vk_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return wlog::WLOG_TRACE;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return wlog::WLOG_INFO;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return wlog::WLOG_WARN;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return wlog::WLOG_ERROR;

        default:
            throw std::runtime_error("Unknown Vulkan log severity bit: " + std::to_string(vk_severity));
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    log(vulkan_log_bit_to_wlog(messageSeverity), pCallbackData->pMessage);

    return VK_FALSE;
}

}

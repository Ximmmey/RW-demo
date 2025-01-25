#pragma once

#include "libgui_vma.h"

#include <vector>
#include <volk.h>

namespace libgui {

/**
 * @brief A VMA allocated buffer.
 */
struct VkSizedBuffer {
    VkDeviceSize size;
    VkBuffer buffer;

    VmaAllocator allocator;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;

    void dispose() const {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
};

/**
 * @brief Creates a sized buffer
 */
inline VkResult create_buffer(const VmaAllocator vma, VkSizedBuffer *buffer, const uint32_t size, const VmaMemoryUsage memory_usage, const VmaAllocationCreateFlags alloc_flags, const VkBufferUsageFlags usage, const VkBufferCreateFlags flags = 0) {
    const VkBufferCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = flags,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    const VmaAllocationCreateInfo alloc {
        .flags = alloc_flags,
        .usage = memory_usage,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    buffer->allocator = vma;
    buffer->size = size;
    return vmaCreateBuffer(vma, &create, &alloc, &buffer->buffer, &buffer->allocation, &buffer->allocation_info);
}

/**
 * @brief Creates a descriptor set layout
 */
inline VkResult descriptor_set_layout(const VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings, VkDescriptorSetLayout *layout, const VkDescriptorSetLayoutCreateFlags flags = 0) {
    const VkDescriptorSetLayoutCreateInfo create_layout {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = flags,

        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    return vkCreateDescriptorSetLayout(device, &create_layout, nullptr, layout);
}

/**
 * @brief Creates an attachment info
 * @param view image view to attach
 * @param layout layout of the image
 * @return The attachment info requested
 */
inline VkRenderingAttachmentInfo attachment_info(const VkImageView view, const VkImageLayout layout) {
    return VkRenderingAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue {},
    };
}

/**
 * @brief Creates a render info
 * @param area Area to render to
 * @param attachment Color attachment to render to
 * @param depth_attachment Depth attachment to render to
 * @param stencil_attachment Stencil attachment to render to
 * @return The render info requested
 */
inline VkRenderingInfo rendering_info(const VkRect2D area, const VkRenderingAttachmentInfo *attachment, const VkRenderingAttachmentInfo *depth_attachment = nullptr, const VkRenderingAttachmentInfo *stencil_attachment = nullptr) {
    return VkRenderingInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = attachment,
        .pDepthAttachment = depth_attachment,
        .pStencilAttachment = stencil_attachment,
    };
}

/**
 * @brief Creates a buffer
 * @param buffer Buffer to wait for
 * @param offset Offset of the event
 * @param src_stage Where the event happened
 * @param src_access How the event accesses
 * @param dst_stage Where the event will end
 * @param dst_access What state we will switch it to
 * @return The barrier requested
 */
inline VkBufferMemoryBarrier2 buffer_memory_barrier(const VkSizedBuffer &buffer, const VkDeviceSize offset, const VkPipelineStageFlags2 src_stage, const VkAccessFlags2 src_access, const VkPipelineStageFlags2 dst_stage, const VkAccessFlags2 dst_access) {
    return VkBufferMemoryBarrier2 {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = src_stage,
        .srcAccessMask = src_access,
        .dstStageMask = dst_stage,
        .dstAccessMask = dst_access,

        .buffer = buffer.buffer,
        .offset = offset,
        .size = buffer.size,
    };
}

// inline VkRenderingAttachmentInfo attachment_info(VkAllocatedImage) SEE VK_UTILS.H, ISSUES WHEN HERE.

/**
 * @brief Creates a command buffer begin info
 */
inline VkCommandBufferBeginInfo command_buffer_begin_info(const VkCommandBufferUsageFlags flags = 0) {
    return VkCommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };
}

/**
 * @brief Creates a clear attachment info
 */
inline VkClearAttachment clear_attachment(const VkImageAspectFlags aspectMask, const VkClearValue clear_color_value) {
    return VkClearAttachment {
        .aspectMask = aspectMask,
        .colorAttachment = 0,
        .clearValue = clear_color_value,
    };
}

/**
 * @brief Creates image subresource range
 */
inline VkImageSubresourceRange image_subresource_range(const VkImageAspectFlags aspectMask) {
    return VkImageSubresourceRange {
        .aspectMask = aspectMask,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

/**
 * @brief Creates a semaphore submit info
 */
inline VkSemaphoreSubmitInfo semaphore_submit_info(const VkPipelineStageFlags2 stageMask, const VkSemaphore semaphore)
{
    return VkSemaphoreSubmitInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask,
        .deviceIndex = 0,
    };
}

/**
 * @brief Creates a present info
 */
inline VkPresentInfoKHR present_info(const VkSwapchainKHR *swapchain_khr, const uint32_t *img_idx, const VkSemaphore *semaphore) {
    return VkPresentInfoKHR {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = semaphore,
        .swapchainCount = 1,
        .pSwapchains = swapchain_khr,
        .pImageIndices = img_idx,
    };
}

/**
 * @brief Creates a command buffer submit info
 */
inline VkCommandBufferSubmitInfo command_buffer_submit_info(const VkCommandBuffer cmd)
{
    return VkCommandBufferSubmitInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
}

/**
 * @brief Creates a queue submit info
 */
inline VkSubmitInfo2 submit_info(const VkCommandBufferSubmitInfo *cmd, const VkSemaphoreSubmitInfo *signal_semaphore, const VkSemaphoreSubmitInfo *wait_semaphore)
{
    return VkSubmitInfo2  {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = wait_semaphore == nullptr ? static_cast<uint32_t>(0) : 1,
        .pWaitSemaphoreInfos = wait_semaphore,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
        .signalSemaphoreInfoCount = signal_semaphore == nullptr ? static_cast<uint32_t>(0) : 1,
        .pSignalSemaphoreInfos = signal_semaphore
    };
}

/**
 * @brief Creates an image create info
 */
inline VkImageCreateInfo image_create_info(const VkFormat format, const VkImageUsageFlags usage, const VkExtent3D extent, const VkImageCreateFlags flags = 0)
{
    return VkImageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,

        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
}

/**
 * @brief Creates an image view create info
 */
inline VkImageViewCreateInfo imageview_create_info(const VkFormat format, const VkImage image, const VkImageAspectFlags aspect)
{
    return VkImageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
}

}

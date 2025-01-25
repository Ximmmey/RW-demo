#pragma once

#include "libgui_init.h"

#include <volk.h>

namespace libgui {
/**
 * @brief DO NOT USE NORMALLY. THIS WILL PAUSE *ALL* BUFFERS AND INVALIDATE *ALL* GPU CACHES. THIS WILL COMPLETELY STALL
 * THE GPU.
 * @param cmd Command Buffer to pause fully
 */
inline void full_pipeline_barrier(const VkCommandBuffer cmd) {
    constexpr VkMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
        .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = 0,

        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for a uniform read event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being read
 * @param offset data offset of the read
 */
inline void uniform_read_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for a uniform write event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being written to
 * @param offset data offset of the write
 */
inline void uniform_write_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for a vertex read event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being read
 * @param offset data offset of the read
 */
inline void vertex_read_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for a vertex write event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being written to
 * @param offset data offset of the write
 */
inline void vertex_write_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for an index read event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being read
 * @param offset data offset of the read
 */
inline void index_read_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_INDEX_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier for an index write event
 * @param cmd command buffer
 * @param updating_buffer the buffer currently being written to
 * @param offset data offset of the write
 */
inline void index_write_barrier(const VkCommandBuffer cmd, const VkSizedBuffer &updating_buffer, const VkDeviceSize offset) {
    const VkBufferMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT,

        .buffer = updating_buffer.buffer,
        .offset = offset,
        .size = updating_buffer.size,
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}

/**
 * @brief inserts a barrier to switch the layout of the given image
 * @param cmd command buffer
 * @param image image to change
 * @param currentLayout current layout of said image, UNDEFINED if you don't know
 * @param newLayout layout to change to
 * @param aspect_mask aspect of the image
 */
static void change_image_layout(const VkCommandBuffer cmd, const VkImage image, const VkImageLayout currentLayout, const VkImageLayout newLayout, const VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT)
{
    const VkImageMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

        .oldLayout = currentLayout,
        .newLayout = newLayout,

        .image = image,
        .subresourceRange = image_subresource_range(aspect_mask),
    };

    const VkDependencyInfo barrier_dep {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &barrier_dep);
}
};

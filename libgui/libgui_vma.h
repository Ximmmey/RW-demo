#pragma once

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000

// Use volk functions and fill those we can't
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include <volk.h>
#include <vk_mem_alloc.h>

namespace libgui {

    /**
     * @brief initialises a VMA allocator
     * @param vk_instance Vulkan instance
     * @param phys_device Physical GPU pointer
     * @param logi_device Vulkan GPU pointer
     * @return VMA instance
     */
static VmaAllocator vma_init(const VkInstance vk_instance, const VkPhysicalDevice phys_device, const VkDevice logi_device) {
    const VmaVulkanFunctions vma_functions = {
        .vkGetInstanceProcAddr                   = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,

    #if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR,
    #endif

    #if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2KHR,
    #endif

    #if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
    #endif

    #if VMA_KHR_MAINTENANCE4 || VMA_VULKAN_VERSION >= 1003000
        .vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements
    #endif
    };

    const VmaAllocatorCreateInfo create {
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,

        .physicalDevice = phys_device,
        .device = logi_device,
        .pVulkanFunctions = &vma_functions,
        .instance = vk_instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    VmaAllocator allocator;
    vmaCreateAllocator(&create, &allocator);

    return allocator;
}

}

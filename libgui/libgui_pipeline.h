#pragma once

#include "libgui_vkutils.h"

#include <volk.h>

#include <span>
#include <vector>

namespace libgui {

    /**
     * @brief A lease of Descriptor Pools.\n
     * Allows you to allocate descriptor sets without worry
     */
class DescriptorLease {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

private:
    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> full;
    std::vector<VkDescriptorPool> ready;
    uint32_t sets_per_pool = 0;

    VkDescriptorPool get_pool(const VkDevice device) {
        VkDescriptorPool get_new;

        // We're out of pools!
        if (!ready.empty()) {
            get_new = ready.back();
            ready.pop_back();
        }
        else {
            get_new = create_pool(device, sets_per_pool, ratios);

            // Clearly it's not going to be enough at this rate, increase set size for next pools
            sets_per_pool = sets_per_pool * 1.5;

            // Upper limit
            if (sets_per_pool > 4092) {
                sets_per_pool = 4092;
            }
        }

        return get_new;
    }

    VkDescriptorPool create_pool(const VkDevice device, const uint32_t set_count, const std::span<PoolSizeRatio> pool_ratios) {
        std::vector<VkDescriptorPoolSize> sizes;

        for (auto [type, ratio]: pool_ratios) {
            sizes.push_back(VkDescriptorPoolSize {
                .type = type,
                .descriptorCount = static_cast<uint32_t>(ratio * set_count),
            });
        }

        const VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = set_count,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data(),
        };

        VkDescriptorPool create;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &create);

        return create;
    }

public:
    void init(const VkDevice device, const uint32_t max_sets, const std::span<PoolSizeRatio> pool_ratios) {
        ratios.clear();

        for (auto r: pool_ratios) {
            ratios.push_back(r);
        }

        const VkDescriptorPool first_pool = create_pool(device, max_sets, pool_ratios);

        sets_per_pool = max_sets * 1.5;

        ready.push_back(first_pool);
    }

    void clear_pools(const VkDevice device) {
        for (const auto p : ready) {
            vkResetDescriptorPool(device, p, 0);
        }

        for (auto p : full) {
            vkResetDescriptorPool(device, p, 0);
            ready.push_back(p);
        }

        full.clear();
    }

    void destroy_pools(const VkDevice device) {
        for (const auto p : ready) {
            vkDestroyDescriptorPool(device, p, nullptr);
        }
        ready.clear();

        for (const auto p : full) {
            vkDestroyDescriptorPool(device,p,nullptr);
        }
        full.clear();
    }

    VkDescriptorSet allocate(const VkDevice device, const VkDescriptorSetLayout layout, void* next = nullptr) {
        // Get or create the pool we will use
        VkDescriptorPool use = get_pool(device);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = next,

            .descriptorPool = use,
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };

        VkDescriptorSet ds;
        const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

        // Whoops, try again
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

            full.push_back(use);

            use = get_pool(device);
            allocInfo.descriptorPool = use;

            VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &ds)); // We really cannot
        }

        ready.push_back(use);
        return ds;
    }
};

/**
 * @brief A helper class for writing data to descriptor sets
 */
class DescriptorLayoutHelper {
    std::deque<VkDescriptorImageInfo> images;
    std::deque<VkDescriptorBufferInfo> buffers;
    std::vector<VkWriteDescriptorSet> writes;

public:
    DescriptorLayoutHelper image(const uint32_t binding, const VkImageView image, const VkSampler sampler, const VkImageLayout layout, const VkDescriptorType type) {
        VkDescriptorImageInfo& info = images.emplace_back(VkDescriptorImageInfo {
            .sampler = sampler,
            .imageView = image,
            .imageLayout = layout,
        });

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,

            .dstSet = VK_NULL_HANDLE, // Used only when we actually write to the set
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pImageInfo = &info,
        };

        writes.push_back(write);

        return *this;
    }

    DescriptorLayoutHelper buffer(const uint32_t binding, const VkBuffer buffer, const size_t size, const size_t offset, const VkDescriptorType type) {
        const VkDescriptorBufferInfo& info = buffers.emplace_back(VkDescriptorBufferInfo {
            .buffer = buffer,
            .offset = offset,
            .range = size,
        });

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,

            .dstSet = VK_NULL_HANDLE, // Used only when we actually write to the set
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pBufferInfo = &info,
        };

        writes.push_back(write);

        return *this;
    }

    DescriptorLayoutHelper clear() {
        images.clear();
        buffers.clear();
        writes.clear();

        return *this;
    }

    DescriptorLayoutHelper update_set(const VkDevice device, const VkDescriptorSet set) {
        for (VkWriteDescriptorSet& write : writes) {
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        return *this;
    }
};

/**
 * @brief A pipeline that includes the required info for a lot of the rendering
 */
struct VkCompletePipeline {
    VkDevice owner_gpu;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    std::span<VkDescriptorSetLayout> set_layouts;

    void dispose() {
        vkDestroyPipeline(owner_gpu, pipeline, nullptr);
        vkDestroyPipelineLayout(owner_gpu, layout, nullptr);

        for (const auto layout: set_layouts) {
            vkDestroyDescriptorSetLayout(owner_gpu, layout, nullptr);
        }
    }
};

/**
 * @brief Builder class for easily creating pipelines
 */
class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    // Vertices
    VkPipelineInputAssemblyStateCreateInfo InputAsm {};
    std::vector<VkVertexInputBindingDescription> VertexBindings {};
    std::vector<VkVertexInputAttributeDescription> VertexAttribs {};

    VkPipelineRasterizationStateCreateInfo Rasterizer {};
    VkPipelineColorBlendAttachmentState ColorBlendAttachment {};
    VkPipelineMultisampleStateCreateInfo Multisampling {};

    // Layouts
    std::vector<VkDescriptorSetLayout> Sets {};

    VkPipelineDepthStencilStateCreateInfo DepthStencil {};
    VkPipelineRenderingCreateInfo RenderInfo {};
    VkFormat ColorAttachmentFormat {};

    PipelineBuilder() { clear(); }

    PipelineBuilder clear() {
        InputAsm = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

        Rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

        ColorBlendAttachment = {};

        Multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

        DepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

        RenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

        ShaderStages.clear();

        return *this;
    }

    PipelineBuilder push_shader(const VkShaderModule module, const VkShaderStageFlagBits stage, const VkSpecializationInfo *specialization_info = nullptr)
    {
        ShaderStages.push_back(VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage,
            .module = module,
            .pName = "main",
            .pSpecializationInfo = specialization_info,
        });

        return *this;
    }

    PipelineBuilder push_layout(const VkDescriptorSetLayout layout) {
        Sets.push_back(layout);

        return *this;
    }

    template <typename T>
    PipelineBuilder push_vertex_binding() {
        const VkVertexInputBindingDescription desc {
            .binding = static_cast<uint32_t>(VertexBindings.size()),
            .stride = sizeof(T),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VertexBindings.push_back(desc);

        return *this;
    }

    PipelineBuilder push_vertex_field(const uint32_t location, const VkFormat format, const uint32_t offset) {
        const VkVertexInputAttributeDescription desc {
            .location = location,
            .binding = static_cast<uint32_t>(VertexBindings.size()),
            .format = format,
            .offset = offset,
        };

        VertexAttribs.push_back(desc);

        return *this;
    }

    PipelineBuilder topology(const VkPrimitiveTopology topology) {
        InputAsm.topology = topology;
        InputAsm.primitiveRestartEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder polygon_mode(const VkPolygonMode mode) {
        Rasterizer.polygonMode = mode;
        Rasterizer.lineWidth = 1.0f;

        return *this;
    }

    PipelineBuilder cull_mode(const VkCullModeFlags mode, const VkFrontFace front) {
        Rasterizer.cullMode = mode;
        Rasterizer.frontFace = front;

        return *this;
    }

    PipelineBuilder multisampling(const VkSampleCountFlagBits sample = VK_SAMPLE_COUNT_1_BIT, const float min_sample = 1.0f, const VkSampleMask *mask = nullptr) {
        Multisampling.sampleShadingEnable = sample != VK_SAMPLE_COUNT_1_BIT;
        Multisampling.rasterizationSamples = sample;
        Multisampling.minSampleShading = min_sample;
        Multisampling.pSampleMask = mask;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder color_blending_none()
    {
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder color_blending_additive()
    {
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_TRUE;

        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        return *this;
    }

    PipelineBuilder color_blending_alpha()
    {
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_TRUE;

        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        return *this;
    }

    PipelineBuilder color_attachment_format(const VkFormat format)
    {
        ColorAttachmentFormat = format;

        RenderInfo.colorAttachmentCount = 1;
        RenderInfo.pColorAttachmentFormats = &ColorAttachmentFormat;

        return *this;
    }

    PipelineBuilder depth_format(const VkFormat format)
    {
        RenderInfo.depthAttachmentFormat = format;

        return *this;
    }

    PipelineBuilder depth_test_none()
    {
        DepthStencil.depthTestEnable = VK_FALSE;
        DepthStencil.depthWriteEnable = VK_FALSE;
        DepthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        DepthStencil.stencilTestEnable = VK_FALSE;
        DepthStencil.front = {};
        DepthStencil.back = {};
        DepthStencil.minDepthBounds = -1.f;
        DepthStencil.maxDepthBounds = 1.f;

        return *this;
    }

    PipelineBuilder depth_test_default(const VkCompareOp compare = VK_COMPARE_OP_GREATER_OR_EQUAL)
    {
        DepthStencil.depthTestEnable = VK_TRUE;
        DepthStencil.depthWriteEnable = VK_TRUE;
        DepthStencil.depthCompareOp = compare;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        DepthStencil.stencilTestEnable = VK_FALSE;
        DepthStencil.front = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = compare,
            .compareMask = 1,
            .writeMask = 1,
        };
        DepthStencil.back = {};
        DepthStencil.minDepthBounds = 0.f;
        DepthStencil.maxDepthBounds = 1.f;

        return *this;
    }

    VkCompletePipeline build(const VkDevice device, const VkPipelineCache cache = VK_NULL_HANDLE) {
        VkPipelineViewportStateCreateInfo viewport = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,

            .viewportCount = 1,
            .scissorCount = 1,
        };

        VkPipelineColorBlendStateCreateInfo blending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,

            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &ColorBlendAttachment,
        };

        VkPipelineVertexInputStateCreateInfo vertex_input = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

            .vertexBindingDescriptionCount = static_cast<uint32_t>(VertexBindings.size()),
            .pVertexBindingDescriptions = VertexBindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexAttribs.size()),
            .pVertexAttributeDescriptions = VertexAttribs.data(),
        };

        // Create pipeline layout
        const VkPipelineLayoutCreateInfo layout_create {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(Sets.size()),
            .pSetLayouts = Sets.data(),
        };

        VkPipelineLayout layout;
        vkCreatePipelineLayout(device, &layout_create, nullptr, &layout);

        VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE };
        VkPipelineDynamicStateCreateInfo dynamic_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,

            .dynamicStateCount = 2,
            .pDynamicStates = dynamic_states,
        };

        // Create the actual pipeline
        const VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &RenderInfo,

            .stageCount = static_cast<uint32_t>(ShaderStages.size()),
            .pStages = ShaderStages.data(),

            .pVertexInputState = &vertex_input,
            .pInputAssemblyState = &InputAsm,

            .pViewportState = &viewport,
            .pRasterizationState = &Rasterizer,
            .pMultisampleState = &Multisampling,
            .pDepthStencilState = &DepthStencil,
            .pColorBlendState = &blending,
            .pDynamicState = &dynamic_state,

            .layout = layout,
        };

        VkPipeline created;
        VK_ASSERT(vkCreateGraphicsPipelines(device, cache, 1, &pipelineInfo, nullptr, &created));

        return { device, layout, created, Sets };
    }
};


}

#pragma once

#include <vector>

class PipelineLease;

// Hash for leased pipelines
typedef std::tuple<VkFormat /*format*/, std::vector<VkDescriptorSetLayout> /*set layouts*/, std::vector<const char*> /*shaders ids*/> LeasePipelineInfo;

// A pipeline, its poller and hash belonging to a lease
struct LeasedPipeline_T {
    PipelineLease &owner_lease;
    LeasePipelineInfo key;
    libgui::VkCompletePipeline pipeline;
    DrawPoller poller;

    explicit LeasedPipeline_T(PipelineLease &owner, const LeasePipelineInfo &key, const libgui::VkCompletePipeline &set_pipeline)
        : owner_lease(owner)
        , key(key)
        , pipeline(set_pipeline)
        , poller()
    {}

    ~LeasedPipeline_T();
};

// Ref counted leased pipeline
typedef std::shared_ptr<LeasedPipeline_T> LeasedPipeline;

// Lease for automatically managing pipelines
class PipelineLease {
public:
    std::map<LeasePipelineInfo, std::weak_ptr<LeasedPipeline_T>> Pipelines {};

    void reset_pollers() {
        for (const auto &pipeline: Pipelines | std::views::values) {
            pipeline.lock()->poller.reset();
        }
    }

    LeasedPipeline ensure_pipeline(VkDevice device, VkFormat format, const std::vector<VkDescriptorSetLayout> &set_layouts, const std::vector<const char*> &shader_ids, const std::vector<std::tuple<VkShaderModule, VkShaderStageFlagBits>> &shaders);
};

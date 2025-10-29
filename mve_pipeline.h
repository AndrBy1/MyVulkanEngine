//some code is borrowed from https://pastebin.com/EmsJWHzb
// -some configInfo stuff
//The pipeline is an object that bundles together all of the GPU's fixed and programmable state needed for drawing or computing. 
// A blueprint that tells GPU how to process input data and produce final output
//

#pragma once

#include "mve_pipeline.h"
#include "mve_device.h"

#include <string>
#include <vector>
namespace mve {
    //contains the data on how we want to configure pipeline, 
    // want this here bc we want application layer code to be easily able to configure pipeline easily and share config between mult pipelines
    struct PipelineConfigInfo {
        PipelineConfigInfo() = default; // allow default construction, needed for C++20
        // disable move constructor. We don't want to accidentally move this object, as it contains Vulkan handles that should not be moved.
		// move constructor is a special constructor that is called when an object is initialized with an rvalue, which is a temporary object that is about to be destroyed
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete; // disable move assignment
        //https://pastebin.com/EmsJWHzb
        // 
        //some shaders expect vertex data to be laid out in a certain way, binding descriptions describe how to read vertex data from vertex buffers, 
        // attribute descriptions describe how to extract vertex attributes from vertex data binding descriptions describe 
        // at which rate to load data from memory throughout the vertices, ex: per-vertex or per-instance attribute descriptions describe 
        // how to handle vertex input attributes within a vertex, ex: position, color, normal, texture coordinates
        //we make these vectors so that we can have multiple bindings and attributes
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class MvePipeline {
    public:
        MvePipeline(MveDevice& device, const std::string& vertFilePath, const std::string& fragFilePath, const PipelineConfigInfo& configInfo);
        ~MvePipeline();

        MvePipeline(const MvePipeline&) = delete; // Disable copy constructor
        MvePipeline& operator=(const MvePipeline&) = delete; // Disable copy assignment

        void bind(VkCommandBuffer commandBuffer);

        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);

    private:
        static std::vector<char> readFile(const std::string& filePath);

        void createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath, const PipelineConfigInfo& configInfo);

        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        MveDevice& mveDevice; // reference to the device, so we can use it to create pipeline
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
} //namespace mve
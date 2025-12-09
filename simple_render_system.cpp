

#include "simple_render_system.h"

#include <stdexcept>

#define GLM_FORCE_RADIANS //always use radians for glm math functions
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //force depth values to be between 0 and 1
#include <glm/glm.hpp> 
#include <glm/gtc/constants.hpp> //for glm::pi

#include <array>
#include <cassert>
#include <iostream>

namespace mve {
	struct SimplePushConstantData { //this matches the push in shaders
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f }; 
	};

	SimpleRenderSystem::SimpleRenderSystem(MveDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts) : mveDevice{ device } {
		if (setLayouts.empty()) {
			throw std::runtime_error("SimpleRenderSystem requires at least one descriptor set layout (global UBO).");
		}

		createPipelineLayout(setLayouts);
		createPipeline(renderPass);
		/*
		for (int i = 0; i < setLayouts.size(); i++) {
		}*/
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(mveDevice.device(), pipelineLayout, nullptr);
		/*
		for (int i = 0; i < pipelineLayouts.size(); i++) {
			
		}*/
	}

	void SimpleRenderSystem::createPipelineLayout(std::vector <VkDescriptorSetLayout> setLayouts) {
		//std::cout << " Creating pipeline layout " << i << "\n";
		//push constant range is a structure that describes a range of push constants. 
		//Range being a section of memory that can be updated frequently and accessed by shaders
		VkPushConstantRange pushConstantRange{}; 
		//specify which shader stages can access the push constant range. We want in both vertex and fragment shaders
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0; //offset is the starting point of the push constant range within the push constant memory
		pushConstantRange.size = sizeof(SimplePushConstantData);

		//descriptor set layouts define the structure of descriptor sets that will be used by the pipeline
		//std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ setLayouts };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		//number of descriptor set layouts being used by the pipeline layout
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		//pointer to an array of descriptor set layouts. 
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		//VkPipelineLayout pLayout;
		if (vkCreatePipelineLayout(mveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
		//pipelineLayouts.push_back(pLayout);
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		//std::cout << " Creating pipeline " << i << "\n";
		assert(pipelineLayout != VK_NULL_HANDLE && "Cannot create pipeline before pipeline layout");
		
		PipelineConfigInfo pipelineConfig{};
		MvePipeline::defaultPipelineConfigInfo(pipelineConfig);//to use swapchain width and height since it might not match the window's
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		mvePipeline = std::make_unique<MvePipeline>(mveDevice, "shader.vert.spv", "shader.frag.spv", pipelineConfig);
		
	}

	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		mvePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
			&frameInfo.globalDescriptorSet, 0, nullptr);

		
		for (auto& obj : frameInfo.gameObjects) {
			//std::cout << " obj:" << obj.first << "\n";
			if (obj.second.model == nullptr) continue;
			//VkDescriptorSet textureSet = obj.second.model->getTextureDescriptor();
			
			VkDescriptorSet textureSet = obj.second.getTextureDescriptor();
			
			if (textureSet != VK_NULL_HANDLE) {
				//texturePipeline->bind(frameInfo.commandBuffer);
				//std::cout << " textureSet" << "\n";

				vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
					&textureSet, 0, nullptr
				);
			}
			SimplePushConstantData push{};
			push.modelMatrix = obj.second.transform.mat4();
			push.normalMatrix = obj.second.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
				sizeof(SimplePushConstantData), &push);

			obj.second.model->bind(frameInfo.commandBuffer);
			obj.second.model->draw(frameInfo.commandBuffer);
		}
	}
}
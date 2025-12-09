//this render system is for rendering simple game objects with a single color shader

#pragma once

#include "mve_camera.h"
#include "mve_pipeline.h"
#include "mve_device.h"
#include "mve_game_object.h"
#include "mve_frame_info.h"

#include <memory>
#include <vector>
#include <string>

namespace mve {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(MveDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts);
		~SimpleRenderSystem();

		//becuase this is managing Vulkan objects for pipeline layout and command buffers, we should delete copy constructors 
		SimpleRenderSystem(const SimpleRenderSystem&) = delete; //disable copy constructor
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete; //disable copy assignment operator

		void renderGameObjects(FrameInfo& frameInfo);
		//VkPipelineLayout& getPipelineLayout() { return pipelineLayouts[0]; }

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayout);
		void createPipeline(VkRenderPass renderPass);

		//order matters here since they are initialized in order listed
		MveDevice& mveDevice;
		std::unique_ptr<MvePipeline> mvePipeline;
		std::unique_ptr<MvePipeline> texturePipeline;
		//std::vector<std::unique_ptr<MvePipeline>> pipelines;
		//std::vector<VkDescriptorSetLayout> setLayouts;
		//std::vector<VkPipelineLayout> pipelineLayouts;
		VkPipelineLayout pipelineLayout;
		//VkPipelineLayout texturedPLayout;
	};
}
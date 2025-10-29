//this render system is for rendering simple game objects with a single color shader

#pragma once

#include "mve_camera.h"
#include "mve_pipeline.h"
#include "mve_device.h"
#include "mve_game_object.h"
#include "mve_frame_info.h"

#include <memory>
#include <vector>

namespace mve {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(MveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		//becuase this is managing Vulkan objects for pipeline layout and command buffers, we should delete copy constructors 
		SimpleRenderSystem(const SimpleRenderSystem&) = delete; //disable copy constructor
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete; //disable copy assignment operator

		void renderGameObjects(FrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		//order matters here since they are initialized in order listed
		MveDevice& mveDevice;
		std::unique_ptr<MvePipeline> mvePipeline;
		VkPipelineLayout pipelineLayout;
	};
}
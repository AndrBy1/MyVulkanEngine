#pragma once

#include "mve_camera.h"
#include "mve_pipeline.h"
#include "mve_device.h"
#include "mve_game_object.h"
#include "mve_frame_info.h"

#include <memory>
#include <vector>

namespace mve {
	class PointLightSystem {
	public:
		PointLightSystem(MveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete; //disable copy constructor
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void update(FrameInfo& frameInfo, GlobalUbo& ubo);
		void render(FrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		//order matters here since they are initialized in order listed
		MveDevice& mveDevice;
		std::unique_ptr<MvePipeline> mvePipeline;
		VkPipelineLayout pipelineLayout;
	};
}
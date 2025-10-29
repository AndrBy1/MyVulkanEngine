//mve_frame_info.h is for passing frame-specific information to various parts of the rendering engine.
//frame-specific information includes data that may change every frame, such as the current frame index, 
// time since last frame, command buffer for the current frame, descriptor sets, camera information, and game objects.
// frames are managed in file mve_swap_chain.h

#pragma once

#include "mve_camera.h"
#include "mve_game_object.h"

#include <vulkan/vulkan.h>

namespace mve {
#define MAX_LIGHTS 10
	struct PointLight {
		glm::vec4 position{}; //vec4 because of std140 alignment requirements in vulkan
		glm::vec4 color{}; //w is intensity
	};

	//Ubo stands for Uniform Buffer Object,
	// global means it is accessible across multiple shaders or stages in the rendering pipeline
	//uniform is a shader variable that remains constant for all processed vertices or fragments during a single draw call
	//buffer is a contiguous block of memory used to store data
	struct GlobalUbo { //UBO is a struct that matches the layout of the uniform buffer in the shader
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::vec4 ambientLight{ 1.f, 1.f, 1.f, .02f };
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo {
		int frameIndex; //current frame index
		float frameTime; //time since last frame
		VkCommandBuffer commandBuffer; //command buffer for the current frame
		MveCamera& camera; //reference to the camera
		VkDescriptorSet globalDescriptorSet; //descriptor set for global UBO
		MveGameObject::Map& gameObjects; //reference to the map of game objects
	};
}
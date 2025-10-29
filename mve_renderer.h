//mve_renderer manages the swapchain, command buffers and draw frame
//command_buffers are buffers or GPU variables that contain instruction or command data
//draw frame are the frames that draw in the window
//simple render system sets up pipeline pipeline layout, simple push constants struct and render game objects

#pragma once
#include "mve_window.h"
#include "mve_device.h"
#include "mve_swap_chain.h"

#include <memory>
#include <vector>
#include <cassert>

namespace mve {
	class MveRenderer {
	public:
		MveRenderer(MveWindow& window, MveDevice& device);
		~MveRenderer();

		//because this is managing Vulkan objects for pipeline layout and command buffers, we should delete copy constructors
		MveRenderer(const MveRenderer&) = delete; //disable copy constructor
		MveRenderer& operator=(const MveRenderer&) = delete;

		//need to be able to access swap chain render pass
		VkRenderPass getSwapChainRenderPass() const { return mveSwapChain->getRenderPass(); }
		float getAspectRatio() const { return mveSwapChain->extentAspectRatio(); }

		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return currentFrameIndex;
		}

		//one function to begin the frame, second to end it
		//beginFrame and beginSwapChainRenderPass isn't combined into a single function because we want application to main control over this functionality
		//so later we can easily integrate shadows, reflection, post processing
		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		//order here matters
		MveWindow& mveWindow;
		MveDevice& mveDevice;
		//use unique pointer can easily create new width and height by constructing a new object
		//unique_ptr is a smart pointer that manages the lifetime of an object
		std::unique_ptr<MveSwapChain> mveSwapChain; 
		std::vector<VkCommandBuffer> commandBuffers;

		//currentImageIndex is the index of the current image in the swap chain that is being rendered to
		//it is used to track which image in the swap chain is currently being used for rendering
		uint32_t currentImageIndex;
		//the current frame index is the index of the current frame being rendered, used to track which command buffer to use
		//the way it is used is that it increments after each frame is rendered and wraps around to 0 when it reaches the max frames in flight
		int currentFrameIndex;
		//isFrameStarted is a boolean flag that indicates whether a frame is currently being rendered or not
		bool isFrameStarted;

	};
}
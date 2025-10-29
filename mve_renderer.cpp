#include "mve_renderer.h"

#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cassert>
#include <iostream>

namespace mve {
	MveRenderer::MveRenderer(MveWindow& window, MveDevice& device) : mveWindow{ window }, mveDevice{ device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	MveRenderer::~MveRenderer() {
		freeCommandBuffers();
	}

	void MveRenderer::recreateSwapChain() {
		auto extent = mveWindow.getExtent(); //getting window width and height
		while (extent.width == 0 || extent.height == 0) {
			extent = mveWindow.getExtent();
			glfwWaitEvents(); //waits until events are qeued and process them
		}
		//vkDeviceWaitIdle Vulkan function that blocks the calling thread until the specified device has completed all of its operations, ensuring that all queued commands are finished executing
		vkDeviceWaitIdle(mveDevice.device());

		if (mveSwapChain == nullptr) {
			mveSwapChain = std::make_unique<MveSwapChain>(mveDevice, extent);
		}
		else {
			//can't copy unique pointer so use std::move
			//std::move is used to enable move semantics which allows data to move from one object to another. from object is left unspecified state. 
			std::shared_ptr<MveSwapChain> oldSwapChain = std::move(mveSwapChain);
			mveSwapChain = std::make_unique<MveSwapChain>(mveDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*mveSwapChain.get())) {
				throw std::runtime_error("Swap chain image (or depth) format has changed!");
			}
		}
		//MveSwapChain = std::make_unique<MveSwapChain>(MveDevice, extent);
	}
	void MveRenderer::createCommandBuffers() {
		commandBuffers.resize(MveSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; //command buffer allocation info
		//primary command buffer can be submitted to a queue for execution, can't be called by other command buffers
		//secondary command buffer can not be submitted but can be colled by other command buffers
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = mveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(mveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void MveRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(mveDevice.device(), mveDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	VkCommandBuffer MveRenderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		//function fetches index of the frame which would render to next
		//handles cpu gpu synchronization surounding double or triple bufferings, return determines if process successful

		auto result = mveSwapChain->acquireNextImage(&currentImageIndex);

		//error can return if window out of size
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr; //frame not started
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//std::cout << "swap_chain commandBuffer: "<< commandBuffers[imageIndex] << std::endl;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		return commandBuffer;
	}

	void MveRenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
		auto commandBuffer = getCurrentCommandBuffer();
		
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
		
		auto result = mveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mveWindow.wasWindowResized()) {
			mveWindow.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		
		currentFrameIndex = (currentFrameIndex + 1) % MveSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void MveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		//VkRenderPassBeginInfo is a struct used to specify parameters when beginning render pass. 
		//render pass groups sequence of rendering ops that share attachments like color and depth buffers, 
		//tells the graphics pipeline what layout to expect for an output frame buffer as well as some other info
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; //render pass begin info
		renderPassInfo.renderPass = mveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = mveSwapChain->getFrameBuffer(currentImageIndex);
		//render area defines the area where the shader will load and stores will take place
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mveSwapChain->getSwapChainExtent();

		//VkClearValue defines the initial clear values, depthStencil is the depth and stencil clear values to use when clearing image or attachment
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		// Begin the render pass
		//VK_SUBPASS_CONTENTS_INLINE signals that the commands will be recorded directly into the primary command buffer, no secondary command buffers will be used
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//viewport describes how coordinates get mapped into final framebuffer space. It defines rectangular region where final image is displayed. 
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(mveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(mveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, mveSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	}

	void MveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);
	}
}
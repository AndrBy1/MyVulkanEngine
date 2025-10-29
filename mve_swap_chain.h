//Swap chain is the queue that manages the presentation of images to the screen. It connects rendered frames to the window system so the user sees them. 
// It holds multiple images (2 or 3) where the GPU renders into one image while another is being displayed. When ready, Vulkan swaps which image is being shown. 
//workflow: Acquire image from swap chain (vkAcquireNextImageKHR), submit rendering commands (vkQueueSubmit), present image (vkQueuePresentKHR)
//
//Image A is currently being displayed on the screen.
//Meanwhile, the GPU is busy rendering the next frame into Image B.
//When rendering finishes and the monitor is ready for the next refresh, Vulkan swaps them:
//Image B is shown on screen.
//Image A goes back into the pool and becomes the next render target.
//Repeat.

#pragma once

#include "mve_device.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <memory>
#include <string>
#include <vector>

namespace mve {

    class MveSwapChain {
    public:
        //constexpr allows declaration of variables, functions and objects that can be evaluated at compile time. 
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2; //limits to this number of command buffers, after these command buffers are submitted, cpu will block call on nextimgage function

        MveSwapChain(MveDevice& deviceRef, VkExtent2D windowExtent);
        MveSwapChain(MveDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<MveSwapChain> previous);
        ~MveSwapChain();

        MveSwapChain(const MveSwapChain&) = delete;
        MveSwapChain& operator=(const MveSwapChain&) = delete;

        VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        VkRenderPass getRenderPass() { return renderPass; }
        VkImageView getImageView(int index) { return swapChainImageViews[index]; }
        size_t imageCount() { return swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() { return swapChainExtent; }
        uint32_t width() { return swapChainExtent.width; }
        uint32_t height() { return swapChainExtent.height; }

        float extentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }
        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t* imageIndex);
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

        bool compareSwapFormats(const MveSwapChain& swapChain) const {
            //when the swap chain is recreated these are the only two values that theoretically may change since the render passes are otherwise created identically
            //so if swap chain depth format and image formats are both the same, the render paths must be compatible
            return swapChain.swapChainDepthFormat == swapChainDepthFormat && swapChain.swapChainImageFormat == swapChainImageFormat;
        }

    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createDepthResources();
        void createRenderPass();
        void createFramebuffers();
        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        MveDevice& device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        //shared_ptr is a smart pointer that allows multiple instances to share ownsership of same obj
        std::shared_ptr<MveSwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;
    };

}  // namespace lve

/*
* Initially based off LittleVulkanEngine
* https://github.com/blurrypiano/littleVulkanEngine
* 
* MveDevice is a class that encapsulates the Vulkan device and related functionalities, such as creating buffers, command pools, and managing memory. 
* It also handles the selection of physical devices and queue families.
* This is required to interact with the GPU and perform rendering operations using Vulkan API.
*/

#pragma once

#include "mve_window.h"

// std lib headers
#include <string>
#include <vector>

namespace mve {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

	struct QueueFamilyIndices { 
        //queue families are different types of queues that a device may support
		uint32_t graphicsFamily; //family of queues that support graphics operations like drawing and image processing
		uint32_t presentFamily; //family of queues that support presentation operations like displaying images to a surface
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    class MveDevice {
    public:
		//Determines whether validation layers should be enabled based on whether the NDEBUG macro is defined.
		//NDEBUG is a standard macro in C/C++ that is typically defined in release builds to disable debugging features and assertions.
		//to turn it on go to project properties -> C/C++ -> Preprocessor -> Preprocessor Definitions and add NDEBUG. Maybe set configuration to all configuration and platform to all platform after
#ifdef NDEBUG
		const bool enableValidationLayers = false; //
#else
        const bool enableValidationLayers = true;
#endif

        MveDevice(MveWindow& window);
        ~MveDevice();

        // Not copyable or movable
        MveDevice(const MveDevice&) = delete;
		MveDevice& operator=(const MveDevice&) = delete; //operator is a special function that is called when an object is assigned a new value from another existing object, in this case, it prevents copying of MveDevice objects
        MveDevice(MveDevice&&) = delete;
        MveDevice& operator=(MveDevice&&) = delete;

		//VkCommandPool is a Vulkan object that manages the memory and lifecycle of command buffers, which are used to record and submit rendering commands to the GPU
        VkCommandPool getCommandPool() { return commandPool; }
		//VkDevice is a logical representation of a physical GPU, used to interact with the GPU and manage resources
        VkDevice device() { return device_; }
		//VkSurfaceKHR is an abstraction for a platform-specific surface that can be used for rendering and presentation. Surface is a Vulkan object that represents a surface to present rendered images to, such as a window or display
        VkSurfaceKHR surface() { return surface_; }
		//VkQueue is a handle to a queue on a device, used to submit command buffers for execution
        VkQueue graphicsQueue() { return graphicsQueue_; }
        VkQueue presentQueue() { return presentQueue_; }

		//SwapChainSupportDetails is a struct that contains information about the swap chain support of a physical device, including its capabilities, supported formats, and present modes
        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
		//findMemoryType is a function that finds a suitable memory type for a given type filter and memory properties
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
        VkFormat findSupportedFormat(
            const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);
		//biginSingleTimeCommands and endSingleTimeCommands are functions that simplify the process of recording and submitting a single-use command buffer for short operations like copying buffers or images
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void createImageWithInfo(
            const VkImageCreateInfo& imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory);

        VkPhysicalDeviceProperties properties;

    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char*> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void hasGflwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        MveWindow& window;
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;

        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

}  // namespace mve
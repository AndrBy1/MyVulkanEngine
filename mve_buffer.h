//The buffer is a contiguous block of memory that holds data such as vertex attributes, indices, or uniform values that shaders can access during rendering.
// 

#pragma once

#include "mve_device.h"

namespace mve {
	class MveBuffer {
	public:
		MveBuffer(MveDevice& device, VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, 
			VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment = 1);
		~MveBuffer();

		MveBuffer(const MveBuffer&) = delete;
		MveBuffer& operator=(const MveBuffer&) = delete;

		//maps memory
		// VkResult is an enumeration that represents the result of a Vulkan operation, indicating success or various types of errors
		//VkDeviceSize size = VK_WHOLE_SIZE means map the entire buffer
		VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();

		//writes to the device (GPU) memory
		void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		//VkDescriptorBufferInfo is a structure that specifies the buffer, offset, and range for a descriptor in Vulkan
		VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		//index varience for each operation. useful for grouping multiple instances into a single buffer rather than using a buffer per instance
		void writeToIndex(void* data, int index);
		VkResult flushIndex(int index);
		VkDescriptorBufferInfo descriptorInfoForIndex(int index);
		VkResult invalidateIndex(int index);

		//getters
		VkBuffer getBuffer() const { return buffer; }
		void* getMappedMemory() const { return mapped; }
		uint32_t getInstanceCount() const { return instanceCount; }
		VkDeviceSize getInstanceSize() const { return instanceSize; }
		VkDeviceSize getAlignmentSize() const { return instanceSize; }
		VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
		VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
		VkDeviceSize getBufferSize() const { return bufferSize; }

	private:

		//minOffsetAlignment similar to fields o a push constant need to follow certain padding guidelines instance of a 
		//uniform block must be at an offset that is at an integer mutiple of the min uniform buffer offset alignment device value
		static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

		MveDevice& mveDevice;
		void* mapped = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize bufferSize;
		uint32_t instanceCount;
		VkDeviceSize instanceSize;
		VkDeviceSize alignmentSize;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;

	};
}
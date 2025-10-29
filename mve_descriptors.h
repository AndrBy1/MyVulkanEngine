//code borrowed from https://pastebin.com/yU7dMAxt
//mve_descriptors is a wrapper around vulkan descriptor sets and layouts
//descriptor sets are used to bind resources to shaders. Descriptor layouts describe the types of resources that will be bound to the shaders
//descriptor pools are used to allocate descriptor sets


#pragma once

#include "mve_device.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mve {
	class MveDescriptorSetLayout {
	public:
		class Builder {
		public:
			Builder(MveDevice& mveDevice) : mveDevice{ mveDevice } {}
			//appends to map of bindings 
			Builder& addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
			//create an instance of the descriptor set layout wrapper class
			std::unique_ptr<MveDescriptorSetLayout> build() const;
		private:
			MveDevice& mveDevice;
			// 
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		MveDescriptorSetLayout(MveDevice& mveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~MveDescriptorSetLayout();
		MveDescriptorSetLayout(const MveDescriptorSetLayout&) = delete;
		MveDescriptorSetLayout& operator=(const MveDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; };

	private:
		MveDevice& mveDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		//friend class allows one class to access private and protected mambers of another class
        friend class mveDescriptorWriter;
	};

	class MveDescriptorPool {
	public:
		class Builder {
		public:
			Builder(MveDevice& mveDevice) : mveDevice{ mveDevice } {}
			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<MveDescriptorPool> build() const;

		private:
			MveDevice& mveDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;

		};
		MveDescriptorPool(MveDevice& mveDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
		~MveDescriptorPool();
		MveDescriptorPool(const MveDescriptorPool&) = delete;
		MveDescriptorPool& operator = (const MveDescriptorPool&) = delete;

		bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

		void resetPool();

	private:
		MveDevice& mveDevice;
		VkDescriptorPool descriptorPool;

		friend class mveDescriptorWriter;
	};

	class mveDescriptorWriter { //make building the descriptor sets easier
	public:
		mveDescriptorWriter(MveDescriptorSetLayout& setLayout, MveDescriptorPool& pool);

		mveDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		mveDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		MveDescriptorSetLayout& setLayout;
		MveDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};
}
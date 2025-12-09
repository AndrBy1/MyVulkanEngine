//this model file takes in .obj files and translates the vertex and indices into visual models. 
#pragma once

#include "mve_image.h"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace mve {
    class MveModel {
    public:

        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
			glm::vec2 uv{}; //uv is texture coordinates which map 2D textures to 3D models

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            //this is used for comparing two Vertex objects to see if they are identical in terms of their attributes, needed for hashing
			//hashing prevents duplicate vertices when loading models
			bool operator==(const Vertex& other) const { 
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
        };
           
        struct Builder {
            std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

            void loadModel(const std::string& filepath);
		};

        MveModel(MveDevice& device, const MveModel::Builder& builder);
        ~MveModel();

        //becuase this is managing Vulkan objects for pipeline layout and command buffers, we should delete copy constructors 
        MveModel(const MveModel&) = delete; // Disable copy constructor
        MveModel& operator=(const MveModel&) = delete;

        static std::unique_ptr<MveModel> createModelFromFile(MveDevice& device, const std::string& filepath);

        VkDescriptorImageInfo attachTextureFromFile(const std::string& filepath);
        //void setTextureDescriptor(VkDescriptorSet descriptor);

		//bind the model's vertex and index buffers to a command buffer so that they can be used for rendering
        void bind(VkCommandBuffer commandBuffer);
		//issue draw commands to render the model using the bound vertex and index buffers
        void draw(VkCommandBuffer commandBuffer);

        MveImage& getTextureImage() { return *textureImage; } //might not need this
        //VkDescriptorSet getTextureDescriptor() const { return textureDescriptor; }
		MveDevice& getDevice() const { return mveDevice; }

    private:
		//these functions create buffers that hold vertex and index data on the GPU
        void createVertexBuffers(const std::vector<Vertex>& vertices); 
		void createIndexBuffers(const std::vector<uint32_t>& indices);

        MveDevice& mveDevice;
        //VkBuffer is a raw block of memory on the GPU (or CPU) that you can use to store any kind of data — vertices, indices, uniform values, staging data, etc.
        std::unique_ptr<MveBuffer> vertexBuffer;
        //VkDeviceMemory is a handle to a block of actual memory allocated from the GPU (or sometimes CPU) for your buffers, images, or other resources.
        uint32_t vertexCount;

        std::unique_ptr<MveImage> textureImage;
        VkDescriptorSet textureDescriptor = VK_NULL_HANDLE;

		bool hasIndexBuffer = false;
        std::unique_ptr<MveBuffer> indexBuffer;
		uint32_t indexCount;
    };
}
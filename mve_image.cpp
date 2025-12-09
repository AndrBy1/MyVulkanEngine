#include "mve_image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef ENGINE_DIR
#define ENGINE_DIR ""
#endif

#include <stdexcept>
#include <iostream> //temporary

namespace mve {
    MveImage::MveImage(MveDevice& device) : mveDevice{ device } {
        //createTextureImage(); this might be better to call from first app to control when image is created
        //
        //createVertexBuffer(); 
        createTextureSampler();
    }

    MveImage::~MveImage() {
        vkDestroySampler(mveDevice.device(), textureSampler, nullptr);
        if (textureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mveDevice.device(), textureImageView, nullptr); //might not need
        }
        if (textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(mveDevice.device(), textureImage, nullptr);
        }
        if (textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mveDevice.device(), textureImageMemory, nullptr);
        }

    }

    void MveImage::createTextureImage(const std::string& imagePath) {

        if (imagePath.empty()) {
            std::cout << "no image path provided\n";

        }

        std::string fullPath = ENGINE_DIR + imagePath;
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        //VkDeviceMemory stagingBufferMemory;

        MveBuffer stagingBuffer(
            mveDevice,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        stagingBuffer.map(imageSize);
        stagingBuffer.writeToBuffer(pixels, static_cast<size_t>(imageSize));
        stagingBuffer.unmap();
        stbi_image_free(pixels);

        createImage(
            texWidth,
            texHeight,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory
        );

        // need to copy the staging buffer to the texture image: 
        // Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Execute the buffer to image copy operation

        //created with VK_IMAGE_LAYOUT_UNDEFINED so that one should be specified as old layout when transitioning the textureImage
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access:
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        createTextureImageView();
        //staging buffer and its memory will be destroyed when it goes out of scope since there is a destructor in AveBuffer

    }
    
    void MveImage::createTextureImageView() {
        //AveSwapChain::createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
        //This needs it's own since swap chain might not be created yet
        mveDevice.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, textureImageView);
        //textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    }

    VkDescriptorImageInfo MveImage::descriptorInfo(VkImageLayout imageLayout) {
        return VkDescriptorImageInfo{
            textureSampler,
            textureImageView,
            imageLayout
        };
    }

    void MveImage::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

        // The parameters for an image creation 
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        // imageType tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed. Possible to create 1D, 2D and 3D images. 
        // 1D images can be used to store an array of data or gradient, 2D images are mainly used for textures, 3D images can be used to store voxel volumes
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        // extent field specifies the dimensions of the image, basically how many texels there are on each axis
        // texels are the individual pixels that make up an image
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        // tiling field can have one of two values:
        // VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
        // VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
        imageInfo.tiling = tiling;
        // possible values for the initialLayout of an image:
        // VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
        // VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels.
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // usage field has the same semantics as the one during buffer creation. 
        // The image is going to be used as destination for the buffer copy, so it should be set up as a transfer destination
        imageInfo.usage = usage;
        // samples flag is related to multisampling. This is only relevant for images that will be used as attachments
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // possible that the VK_FORMAT_R8G8B8A8_SRGB format is not supported by the graphics hardware
        if (vkCreateImage(mveDevice.device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // Allocating memory for an image works in exactly the same way as allocating memory for a buffer. 
        // Use vkGetImageMemoryRequirements instead of vkGetBufferMemoryRequirements, and use vkBindImageMemory instead of vkBindBufferMemory.
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(mveDevice.device(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = mveDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(mveDevice.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(mveDevice.device(), image, imageMemory, 0);
    }

    void MveImage::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = mveDevice.beginSingleTimeCommands();
        // VkImageMemoryBarrier is the structure that defines a memory barrier for an image resource.
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        // specify layout transition. It is possible to use VK_IMAGE_LAYOUT_UNDEFINED as oldLayout if you don't care about the existing contents of the image.
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.image = image;
        // be set to VK_QUEUE_FAMILY_IGNORED if the ownership is not being transferred
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // The image and subresourceRange specify the image that is affected and the specific part of the image.
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        //VkPipelineStageFlags is used to specify the pipeline stages involved in the memory barrier operation.
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        mveDevice.endSingleTimeCommands(commandBuffer);
    }

    void MveImage::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = mveDevice.beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        mveDevice.endSingleTimeCommands(commandBuffer);
    }

    //
    void MveImage::createTextureSampler() {

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // magFilter and minFilter fields specify how to interpolate texels that are magnified or minified
        // choices are VK_FILTER_NEAREST and VK_FILTER_LINEAR
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
        // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
        // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the image.
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        // maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = mveDevice.properties.limits.maxSamplerAnisotropy;
        // It is possible to return black, white or transparent in either float or int formats
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image. 
        // If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range. 
        // If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        // If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison is used in filtering operations.
        // This is mainly used for percentage-closer filtering on shadow maps
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        // 
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(mveDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

    }

}
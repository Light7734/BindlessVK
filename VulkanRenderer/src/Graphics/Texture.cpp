#include "Graphics/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(TextureCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Load the image
	uint8_t* imageData = stbi_load(createInfo.imagePath.c_str(), &m_Width, &m_Height, &m_Channels, 4u);
	ASSERT(imageData, "Failed to load image at: {}", createInfo.imagePath);
	m_ImageSize = m_Width * m_Height * 4u;

	/////////////////////////////////////////////////////////////////////////////////
	// Create the image and staging buffer
	{
		// Image
		VkImageCreateInfo imageCreateInfo {
			.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags     = 0x0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format    = VK_FORMAT_R8G8B8A8_SRGB,
			.extent    = {
			       .width  = static_cast<uint32_t>(m_Width),
			       .height = static_cast<uint32_t>(m_Height),
			       .depth  = 1u,
            },
			.mipLevels     = 1u,
			.arrayLayers   = 1u,
			.samples       = VK_SAMPLE_COUNT_1_BIT,
			.tiling        = VK_IMAGE_TILING_OPTIMAL,
			.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VKC(vkCreateImage(m_LogicalDevice, &imageCreateInfo, nullptr, &m_Image));

		// Staging buffer
		VkBufferCreateInfo stagingBufferCrateInfo {
			.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size        = m_ImageSize,
			.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VKC(vkCreateBuffer(m_LogicalDevice, &stagingBufferCrateInfo, nullptr, &m_StagingBuffer));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Alloacte and bind the image and buffer memory
	{
		// Fetch memory requirements
		VkMemoryRequirements imageMemReq;
		VkMemoryRequirements bufferMemReq;
		vkGetImageMemoryRequirements(m_LogicalDevice, m_Image, &imageMemReq);
		vkGetBufferMemoryRequirements(m_LogicalDevice, m_StagingBuffer, &bufferMemReq);

		// Fetch device memory properties
		VkPhysicalDeviceMemoryProperties physicalMemProps;
		vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);

		// Find adequate memories' indices
		uint32_t imageMemTypeIndex  = UINT32_MAX;
		uint32_t bufferMemTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (imageMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
			{
				imageMemTypeIndex = i;
			}

			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				bufferMemTypeIndex = i;
			}
		}

		ASSERT(imageMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");
		ASSERT(bufferMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

		// Alloacte memory
		VkMemoryAllocateInfo imageMemAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = imageMemReq.size,
			.memoryTypeIndex = imageMemTypeIndex,
		};
		VkMemoryAllocateInfo bufferMemAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = bufferMemReq.size,
			.memoryTypeIndex = bufferMemTypeIndex,
		};

		// Bind memory
		VKC(vkAllocateMemory(m_LogicalDevice, &imageMemAllocInfo, nullptr, &m_ImageMemory));
		VKC(vkBindImageMemory(m_LogicalDevice, m_Image, m_ImageMemory, 0u));

		VKC(vkAllocateMemory(m_LogicalDevice, &bufferMemAllocInfo, nullptr, &m_StagingBufferMemory));
		VKC(vkBindBufferMemory(m_LogicalDevice, m_StagingBuffer, m_StagingBufferMemory, 0u));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write the image data
	{
		// Copy data to staging buffer
		void* stagingMap;
		VKC(vkMapMemory(m_LogicalDevice, m_StagingBufferMemory, 0u, m_ImageSize, 0u, &stagingMap));
		memcpy(stagingMap, imageData, m_ImageSize);
		vkUnmapMemory(m_LogicalDevice, m_StagingBufferMemory);

		// Allocate cmd buffer
		VkCommandBufferAllocateInfo allocInfo {
			.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool        = createInfo.commandPool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1u,
		};
		VkCommandBuffer cmdBuffer;
		VKC(vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &cmdBuffer));

		// Record cmd buffer
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		VKC(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

		// Move data from staging buffer to image
		TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(cmdBuffer);
		TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// End & Submit cmd buffer
		VKC(vkEndCommandBuffer(cmdBuffer));
		VkSubmitInfo submitInfo {
			.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1u,
			.pCommandBuffers    = &cmdBuffer,
		};
		VKC(vkQueueSubmit(createInfo.graphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE));

		// Wait for and free cmd buffer
		VKC(vkQueueWaitIdle(createInfo.graphicsQueue));
		vkFreeCommandBuffers(m_LogicalDevice, createInfo.commandPool, 1u, &cmdBuffer);


		// Free image data
		stbi_image_free(imageData);
	}
}

Texture::~Texture()
{
	vkDestroyBuffer(m_LogicalDevice, m_StagingBuffer, nullptr);
	vkFreeMemory(m_LogicalDevice, m_StagingBufferMemory, nullptr);

	vkDestroyImage(m_LogicalDevice, m_Image, nullptr);
	vkFreeMemory(m_LogicalDevice, m_ImageMemory, nullptr);
}

void Texture::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// Memory barrier
	VkImageMemoryBarrier imageMemBarrier {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout           = oldLayout,
		.newLayout           = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = m_Image,
		.subresourceRange {
		    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		    .baseMipLevel   = 0u,
		    .levelCount     = 1u,
		    .baseArrayLayer = 0u,
		    .layerCount     = 1u,
		},
	};

	/* Specify the source & destination stages and access masks... */
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	// Undefined -> Transfer
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemBarrier.srcAccessMask = 0x0;
		imageMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	// Transfer -> Shader read only
	if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	// Execute pipeline barrier
	vkCmdPipelineBarrier(cmdBuffer,
	                     srcStage, dstStage,
	                     0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &imageMemBarrier);
}

void Texture::CopyBufferToImage(VkCommandBuffer cmdBuffer)
{
	VkBufferImageCopy bufferImageCopy {
		.bufferOffset      = 0u,
		.bufferRowLength   = 0u,
		.bufferImageHeight = 0u,
		.imageSubresource  = {
		     .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		     .mipLevel       = 0u,
		     .baseArrayLayer = 0u,
		     .layerCount     = 1u,
        },
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1u },
	};

	vkCmdCopyBufferToImage(cmdBuffer, m_StagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &bufferImageCopy);
}

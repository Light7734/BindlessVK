#pragma once

#include "Core/Base.hpp"

#include <volk.h>

struct TextureCreateInfo
{
	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	VkQueue graphicsQueue;

	VkCommandPool commandPool;

	const std::string imagePath;
};

class Texture
{
public:
	Texture(TextureCreateInfo& createInfo);
	~Texture();

private:
	void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkCommandBuffer cmdBuffer);

private:
	VkDevice m_LogicalDevice;

	int m_Width, m_Height, m_Channels;
	VkDeviceSize m_ImageSize;

	VkImage m_Image;
	VkDeviceMemory m_ImageMemory;

	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_StagingBufferMemory;
};

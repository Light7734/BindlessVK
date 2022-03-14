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
	bool anisotropyEnabled;
	float maxAnisotropy;
};

class Texture
{
public:
	Texture(TextureCreateInfo& createInfo);
	~Texture();

	inline VkImageView GetImageView() { return m_ImageView; }
	inline VkSampler GetSampler() { return m_Sampler; }

private:
	void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkCommandBuffer cmdBuffer);

private:
	VkDevice m_LogicalDevice;

	int m_Width, m_Height, m_Channels;
	VkDeviceSize m_ImageSize;

	VkImage m_Image              = VK_NULL_HANDLE;
	VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;

	VkImageView m_ImageView = VK_NULL_HANDLE;
	VkSampler m_Sampler     = VK_NULL_HANDLE;

	VkBuffer m_StagingBuffer             = VK_NULL_HANDLE;
	VkDeviceMemory m_StagingBufferMemory = VK_NULL_HANDLE;
};

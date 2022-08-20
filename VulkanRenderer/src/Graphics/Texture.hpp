#pragma once

#include "Core/Base.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

struct TextureCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vk::Queue graphicsQueue;

	vk::CommandPool commandPool;

	const std::string imagePath;
	bool anisotropyEnabled;
	float maxAnisotropy;
};

class Texture
{
public:
	Texture(TextureCreateInfo& createInfo);
	~Texture();

	inline vk::ImageView GetImageView() { return m_ImageView; }
	inline vk::Sampler GetSampler() { return m_Sampler; }

private:
	void TransitionLayout(vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CopyBufferToImage(vk::CommandBuffer cmdBuffer);

private:
	vk::Device m_LogicalDevice;

	int m_Width, m_Height, m_Channels;
	uint32_t m_MipLevels;
	vk::DeviceSize m_ImageSize;

	vk::Image m_Image              = VK_NULL_HANDLE;
	vk::DeviceMemory m_ImageMemory = VK_NULL_HANDLE;

	vk::ImageView m_ImageView = VK_NULL_HANDLE;
	vk::Sampler m_Sampler     = VK_NULL_HANDLE;

	vk::Buffer m_StagingBuffer             = VK_NULL_HANDLE;
	vk::DeviceMemory m_StagingBufferMemory = VK_NULL_HANDLE;
};

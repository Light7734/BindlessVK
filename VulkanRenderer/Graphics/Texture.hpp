#pragma once

#include "Core/Base.hpp"
#include "Graphics/Types.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

struct TextureCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vk::Queue graphicsQueue;

	vma::Allocator allocator;

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
	vk::Device m_LogicalDevice = {};

	vma::Allocator m_Allocator = {};

	int m_Width                = {};
	int m_Height               = {};
	int m_Channels             = {};
	uint32_t m_MipLevels       = {};
	vk::DeviceSize m_ImageSize = {};

	AllocatedImage m_Image    = {};
	vk::ImageView m_ImageView = {};
	vk::Sampler m_Sampler     = {};

	AllocatedBuffer m_StagingBuffer = {};
};

#pragma once

#include "Core/Base.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Types.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

struct Texture
{
	struct CreateInfo
	{
		const std::string& name;
		const std::string& uri;
		uint8_t* pixels;
		int width;
		int height;
		vk::DeviceSize size;
	};

	vk::DescriptorImageInfo descriptorInfo;

	uint32_t width;
	uint32_t height;
	uint32_t channels;
	uint32_t mipLevels;
	vk::DeviceSize size;

	AllocatedImage image;
	vk::ImageView imageView;
	vk::Sampler sampler;

	AllocatedBuffer buffer;
};


class TextureSystem
{
public:
	struct CreateInfo
	{
		DeviceContext deviceContext;
	};

public:
	TextureSystem(const TextureSystem::CreateInfo& info);
	TextureSystem() = default;
	~TextureSystem();

	void Init();

	Texture* CreateTexture(const Texture::CreateInfo& info);
	inline Texture* GetTexture(const char* name) { return &m_Textures[HashStr(name)]; }

private:
	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);
	void TransitionLayout(Texture* texture, vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CopyBufferToImage(Texture* texture, vk::CommandBuffer cmdBuffer);

private:
	struct UploadContext
	{
		vk::CommandBuffer cmdBuffer;
		vk::CommandPool cmdPool;
		vk::Fence fence;
	} m_UploadContext = {};

private:
	vk::Device m_LogicalDevice                         = {};
	vk::PhysicalDevice m_PhysicalDevice                = {};
	vk::PhysicalDeviceProperties m_PhysicalDeviceProps = {};
	vk::Queue m_GraphicsQueue;

	vma::Allocator m_Allocator = {};

	std::unordered_map<uint64_t, Texture> m_Textures = {};
};

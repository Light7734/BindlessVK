#pragma once

#include "Core/Base.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Types.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace tinygltf {
struct Image;
}

struct Texture
{
	struct CreateInfoGLTF
	{
		struct tinygltf::Image* image;
	};

	/// @todo: Implement loading textures from glb files
	struct CreateInfoGLB
	{
	};

	/// @todo: Implement loading textures from ktx files
	struct CreateInfoKTX
	{
	};

	struct CreateInfoData
	{
		const std::string& name;
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

	Texture* CreateFromGLTF(const Texture::CreateInfoGLTF& info);

	Texture* CreateFromKTX(const Texture::CreateInfoKTX& info)
	{
		LOG(err, "NO IMPLEMENT!");
		return {};
	}

	Texture* CreateFromGLB(const Texture::CreateInfoGLB& info)
	{
		LOG(err, "NO IMPLEMENT!");
		return {};
	}

	Texture* CreateFromData(const Texture::CreateInfoData& info);

	inline Texture* GetTexture(const char* name)
	{
		return &m_Textures[HashStr(name)];
	}

private:
	void BlitImage(vk::CommandBuffer cmd, AllocatedImage image, uint32_t mipIndex, int32_t& mipWidth, int32_t& mipHeight);

private:
	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);
	void TransitionLayout(Texture& texture, vk::CommandBuffer cmdBuffer, uint32_t baseMipLevel, uint32_t levelCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CopyBufferToImage(Texture& texture, vk::CommandBuffer cmdBuffer);

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

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
	enum class Type
	{
		e2D,
		e2DArray,
		eCubeMap
	};

	struct CreateInfoGLTF
	{
		struct tinygltf::Image* image;
		vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	};

	/// @todo: Implement loading textures from glb files
	struct CreateInfoGLB
	{
	};

	/// @todo: Implement loading textures from ktx files
	struct CreateInfoKTX
	{
		const std::string& name;
		const std::string& uri;
		Type type;

		vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	};

	struct CreateInfoBuffer
	{
		const std::string& name;
		uint8_t* pixels;
		int width;
		int height;
		vk::DeviceSize size;
		Type type;

		vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	};


	vk::DescriptorImageInfo descriptorInfo;

	uint32_t width;
	uint32_t height;
	vk::Format format;
	uint32_t channels;
	uint32_t mipLevels;
	vk::DeviceSize size;

	vk::Sampler sampler;
	vk::ImageView imageView;
	vk::ImageLayout layout;

	AllocatedImage image;
};

struct TextureCube
{
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

	Texture* CreateFromBuffer(const Texture::CreateInfoBuffer& info);

	Texture* CreateFromGLTF(const Texture::CreateInfoGLTF& info);

	Texture* CreateFromKTX(const Texture::CreateInfoKTX& info);

	Texture* CreateFromGLB(const Texture::CreateInfoGLB& info)
	{
		LOG(err, "NO IMPLEMENT!");
		return {};
	}

	inline Texture* GetTexture(const char* name)
	{
		return &m_Textures[HashStr(name)];
	}

private:
	void BlitImage(vk::CommandBuffer cmd, AllocatedImage image, uint32_t mipIndex, int32_t& mipWidth, int32_t& mipHeight);

private:
	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);
	void TransitionLayout(Texture& texture, vk::CommandBuffer cmdBuffer, uint32_t baseMipLevel, uint32_t levelCount, uint32_t layerCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
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

#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/**
 * @todo Should image and sampler be in separate structs?
 */
class Texture
{
private:
	friend class TextureLoader;
	friend class BinaryLoader;
	friend class KtxLoader;

public:
	enum class Type : u8
	{
		e2D,
		e2DArray,
		eCubeMap
	};

public:
	~Texture();

	Texture(Texture &&);
	Texture &operator=(Texture &&);

	Texture(Texture const &) = delete;
	Texture &operator=(const Texture &) = delete;

	inline auto get_name() const
	{
		return str_view(debug_name);
	}

	inline auto *get_descriptor_info() const
	{
		return &descriptor_info;
	}

	inline auto &get_size() const
	{
		return size;
	}

	inline auto get_format() const
	{
		return format;
	}

	inline auto get_device_size() const
	{
		return device_size;
	}

	inline auto get_sampler() const
	{
		return sampler;
	}

	inline auto get_image_view() const
	{
		return image_view;
	}

	inline auto get_current_layout() const
	{
		return current_layout;
	}

	inline auto get_image() const
	{
		return image;
	}

	void transition_layout(
	    VkContext const *const vk_context,
	    vk::CommandBuffer cmd,
	    u32 base_mip_level,
	    u32 level_count,
	    u32 layer_count,
	    vk::ImageLayout new_layout
	);

	void blit(vk::CommandBuffer cmd, u32 mip_index, pair<i32, i32> mip_size);

private:
	Texture() = default;

	VkContext const *vk_context;

	// @todo: should we delete sampler,image_view,current_layout because of this?
	vk::DescriptorImageInfo descriptor_info = {};

	pair<u32, u32> size = {};
	vk::Format format = {};
	u32 mip_levels = {};
	vk::DeviceSize device_size = {};

	AllocatedImage image = {};

	vk::Sampler sampler = {};
	vk::ImageView image_view = {};
	vk::ImageLayout current_layout = vk::ImageLayout::eUndefined;

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE

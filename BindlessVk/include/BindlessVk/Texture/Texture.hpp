#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Texture/Image.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Repersents the textue data needed for texture-shading an object */
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
	/** Default move constructor */
	Texture(Texture &&) = default;

	/** Default move assignment operator */
	Texture &operator=(Texture &&) = default;

	/** Deleted copy constructor */
	Texture(Texture const &) = delete;

	/** Deleted copy assignment operator  */
	Texture &operator=(const Texture &) = delete;

	/** Destructor */
	~Texture();

	/** Transitions the image layout of specified subresource range to new_layout
	 *
	 * @param cmd Command buffer to record the pipelineBarrier command to
	 * @param base_mip_level First mipmap level to transition the layout of
	 * @param level_count Number of mipmap levels(starting from base_mip_level) to transition the
	 * layout of
	 * @param layer_count Number of array layers to transition the layout of
	 * @param new_layout Desired layout to transition to
	 */
	void transition_layout(
	    vk::CommandBuffer cmd,
	    u32 base_mip_level,
	    u32 level_count,
	    u32 layer_count,
	    vk::ImageLayout new_layout
	);

	/** Blits the image to create a mipmap level
	 *
	 * @param cmd The command buffer to record the blitImage command to
	 * @param mip_index Destination mip level to blit into
	 * @param mip_size Source mip's dimensions
	 */
	void blit(vk::CommandBuffer cmd, u32 mip_index, pair<i32, i32> mip_size);

	/** Returns null terminated str view to debug_name */
	auto get_name() const
	{
		return str_view(debug_name);
	}

	/** Trivial address-accessor for image */
	auto get_image() const
	{
		return &image;
	}

	/** Trivial accessor for descriptor_info */
	auto get_descriptor_info() const
	{
		return &descriptor_info;
	}

	/** Trivial accessor for size */
	auto get_size() const
	{
		return size;
	}

	/** Trivial accessor for format */
	auto get_format() const
	{
		return format;
	}

	/** Trivial accessor for device_size */
	auto get_device_size() const
	{
		return device_size;
	}

	/** Trivial accessor for sampler  */
	auto get_sampler() const
	{
		return sampler;
	}

	/** Trivial accessor for image_view */
	auto get_image_view() const
	{
		return image_view;
	}

	/** Trivial accessor for current_layout */
	auto get_current_layout() const
	{
		return current_layout;
	}

private:
	Texture() = default;

	tidy_ptr<Device const> device = {};

	DebugUtils const *debug_utils = {};
	MemoryAllocator const *memory_allocator = {};

	Image image = {};

	pair<u32, u32> size = {};
	vk::Format format = {};
	u32 mip_levels = {};
	vk::DeviceSize device_size = {};

	vk::Sampler sampler = {};
	vk::ImageView image_view = {};
	vk::ImageLayout current_layout = vk::ImageLayout::eUndefined;

	vk::DescriptorImageInfo descriptor_info = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE

#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Texture/Texture.hpp"

namespace BINDLESSVK_NAMESPACE {

class BinaryLoader
{
public:
	BinaryLoader(VkContext const *const vk_context, Buffer *staging_buffer);

	Texture load(
	    u8 const *const pixels,
	    u32 width,
	    u32 height,
	    vk::DeviceSize size,
	    Texture::Type type,
	    vk::ImageLayout final_layout,
	    str_view debug_name
	);

private:
	void create_image();
	void create_image_view();
	void create_sampler();

	void create_mipmaps(vk::CommandBuffer cmd);

	void stage_texture_data(u8 const *const pixels, vk::DeviceSize size);
	void write_texture_data_to_gpu();

private:
	VkContext const *const vk_context = {};
	Buffer *const staging_buffer = {};

	Texture texture = {};
};

} // namespace BINDLESSVK_NAMESPACE

#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Texture.hpp"

namespace BINDLESSVK_NAMESPACE {

class BinaryLoader
{
public:
	BinaryLoader(Device* device, Buffer* staging_buffer);

	Texture load(
	    c_str name,
	    u8* pixels,
	    u32 width,
	    u32 height,
	    vk::DeviceSize size,
	    Texture::Type type,
	    vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

private:
	void create_image();
	void create_image_view();
	void create_sampler();

	void create_mipmaps(vk::CommandBuffer cmd);

	void stage_texture_data(u8* pixels, vk::DeviceSize size);
	void write_texture_data_to_gpu();


private:
	Device* device = {};
	Buffer* staging_buffer = {};

	Texture texture = {};
};

} // namespace BINDLESSVK_NAMESPACE

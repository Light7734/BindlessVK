#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Texture/Texture.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>

namespace BINDLESSVK_NAMESPACE {

class KtxLoader
{
public:
	KtxLoader(
	    VkContext const *vk_context,
	    MemoryAllocator const *memory_allocator,
	    Buffer *staging_buffer
	);

	Texture load(str_view path, Texture::Type type, vk::ImageLayout final_layout, str_view name);

private:
	void load_ktx_texture(str_view path);
	void destroy_ktx_texture();

	void stage_texture_data();
	void write_texture_data_to_gpu(vk::ImageLayout final_layout);

	void create_image();
	void create_image_view();
	void create_sampler();

	auto create_texture_face_buffer_copies() -> vec<vk::BufferImageCopy>;

private:
	Device const *device {};
	MemoryAllocator const *memory_allocator {};

	Buffer *const staging_buffer = {};

	Texture texture = {};
	ktxTexture *ktx_texture = {};
};

} // namespace BINDLESSVK_NAMESPACE

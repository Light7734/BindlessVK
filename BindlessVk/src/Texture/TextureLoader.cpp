#include "BindlessVk/Texture/TextureLoader.hpp"


#include "BindlessVk/Texture/Loaders/BinaryLoader.hpp"
#include "BindlessVk/Texture/Loaders/KtxLoader.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

TextureLoader::TextureLoader(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator
)
    : vk_context(vk_context)
    , memory_allocator(memory_allocator)
{
	ZoneScoped;

	auto const gpu = vk_context->get_gpu();

	// @todo move this assertion to a proper place
	assert_true(
	    gpu->vk().getFormatProperties(vk::Format::eR8G8B8A8Srgb).optimalTilingFeatures
	        & vk::FormatFeatureFlagBits::eSampledImageFilterLinear,
	    "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);
}

auto TextureLoader::load_from_binary(
    u8 const *const pixels,
    i32 const width,
    i32 const height,
    vk::DeviceSize const size,
    Texture::Type const type,
    Buffer *const staging_buffer,
    vk::ImageLayout const final_layout, /* = vk::ImageLayout::eShaderReadOnlyOptimal */
    str_view const debug_name           /* = default_debug_name */
) const -> Texture
{
	ZoneScoped;

	BinaryLoader loader(vk_context, memory_allocator, staging_buffer);
	return std::move(loader.load(pixels, width, height, size, type, final_layout, debug_name));
}

auto TextureLoader::load_from_ktx(
    str_view const uri,
    Texture::Type const type,
    Buffer *const staging_buffer,
    vk::ImageLayout const layout, /* = vk::ImageLayout::eShaderReadOnlyOptimal */
    str_view const debug_name     /* = default_debug_name */
) const -> Texture
{
	ZoneScoped;

	KtxLoader loader(vk_context, memory_allocator, staging_buffer);
	return std::move(loader.load(uri, type, layout, debug_name));
}

} // namespace BINDLESSVK_NAMESPACE

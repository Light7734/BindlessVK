#include "BindlessVk/TextureLoader.hpp"

#include "BindlessVk/TextureLoaders/BinaryLoader.hpp"
#include "BindlessVk/TextureLoaders/KtxLoader.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

TextureLoader::TextureLoader(ref<VkContext const> const vk_context): vk_context(vk_context)
{
	const auto gpu = vk_context->get_gpu();

	// @todo move this assertion to a proper place
	assert_true(
	    gpu.get_format_properties(vk::Format::eR8G8B8A8Srgb).optimalTilingFeatures &
	        vk::FormatFeatureFlagBits::eSampledImageFilterLinear,
	    "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);
}

auto TextureLoader::load_from_binary(
    c_str const name,
    u8 const *const pixels,
    i32 const width,
    i32 const height,
    vk::DeviceSize const size,
    Texture::Type const type,
    Buffer *const staging_buffer,
    vk::ImageLayout const final_layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
) const -> Texture
{
	BinaryLoader loader(vk_context.get(), staging_buffer);
	return loader.load(name, pixels, width, height, size, type, final_layout);
}

auto TextureLoader::load_from_ktx(
    c_str const name,
    c_str const uri,
    Texture::Type const type,
    Buffer *const staging_buffer,
    vk::ImageLayout const layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
) const -> Texture
{
	KtxLoader loader(vk_context.get(), staging_buffer);
	return loader.load(name, uri, type, layout);
}

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/TextureLoader.hpp"

#include "BindlessVk/TextureLoaders/BinaryLoader.hpp"
#include "BindlessVk/TextureLoaders/KtxLoader.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

TextureLoader::TextureLoader(VkContext* vk_context): vk_context(vk_context)
{
	const auto gpu = vk_context->get_gpu();
	const auto& queues = vk_context->get_queues();

	assert_true(
	    gpu.getFormatProperties(vk::Format::eR8G8B8A8Srgb).optimalTilingFeatures
	        & vk::FormatFeatureFlagBits::eSampledImageFilterLinear,
	    "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);

	vk::CommandPoolCreateInfo commnad_pool_create_info {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queues.graphics_index,
	};
}

Texture TextureLoader::load_from_binary(
    c_str name,
    u8* pixels,
    i32 width,
    i32 height,
    vk::DeviceSize size,
    Texture::Type type,
    Buffer* staging_buffer,
    vk::ImageLayout final_layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	BinaryLoader loader(vk_context, staging_buffer);
	return loader.load(name, pixels, width, height, size, type, final_layout);
}

Texture TextureLoader::load_from_ktx(
    c_str name,
    c_str uri,
    Texture::Type type,
    Buffer* staging_buffer,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	KtxLoader loader(vk_context, staging_buffer);
	return loader.load(name, uri, type, layout);
}

} // namespace BINDLESSVK_NAMESPACE

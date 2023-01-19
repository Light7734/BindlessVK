#include "BindlessVk/TextureLoader.hpp"

#include "BindlessVk/TextureLoaders/KtxLoader.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

static_assert(KTX_SUCCESS == false, "KTX_SUCCESS was supposed to be 0 (false), but it isn't");

namespace BINDLESSVK_NAMESPACE {

TextureLoader::TextureLoader(Device* device): device(device)
{
	assert_true(
	    device->physical.getFormatProperties(vk::Format::eR8G8B8A8Srgb).optimalTilingFeatures
	        & vk::FormatFeatureFlagBits::eSampledImageFilterLinear,
	    "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);

	vk::CommandPoolCreateInfo commnad_pool_create_info {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		device->graphics_queue_index,
	};
}

Texture TextureLoader::load_from_buffer(
    const std::string& name,
    u8* pixels,
    i32 width,
    i32 height,
    vk::DeviceSize size,
    Texture::Type type,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	Texture texture = {
		.descriptor_info = {},
		.width = static_cast<u32>(width),
		.height = static_cast<u32>(height),
		.format = vk::Format::eR8G8B8A8Srgb,
		.mip_levels = static_cast<u32>(std::floor(std::log2(std::max(width, height))) + 1),
		.size = size,
		.sampler = {},
		.image_view = {},
		.current_layout = vk::ImageLayout::eUndefined,
		.image = {},
	};

	/////////////////////////////////////////////////////////////////////////////////
	// Create the image and staging buffer
	{
		// Create vulkan image
		vk::ImageCreateInfo image_create_info {
			{},
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Srgb,
			vk::Extent3D {
			    texture.width,
			    texture.height,
			    1u,
			},
			texture.mip_levels,
			1u,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
			    | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			0u,
			nullptr,
			vk::ImageLayout::eUndefined,
		};

		vma::AllocationCreateInfo image_allocate_info(
		    {},
		    vma::MemoryUsage::eGpuOnly,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		texture.image = device->allocator.createImage(image_create_info, image_allocate_info);

		// Create staging buffer
		vk::BufferCreateInfo staging_buffer_create_info {
			{},
			size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
		};

		vma::AllocationCreateInfo buffer_allocate_info(
		    {},
		    vma::MemoryUsage::eCpuOnly,
		    { vk::MemoryPropertyFlagBits::eHostCached }
		);

		AllocatedBuffer staging_buffer;
		staging_buffer =
		    device->allocator.createBuffer(staging_buffer_create_info, buffer_allocate_info);

		// Copy data to staging buffer
		memcpy(device->allocator.mapMemory(staging_buffer), pixels, size);
		device->allocator.unmapMemory(staging_buffer);

		device->immediate_submit([&](vk::CommandBuffer cmd) {
			texture.transition_layout(
			    device,
			    cmd,
			    0u,
			    texture.mip_levels,
			    1u,
			    vk::ImageLayout::eTransferDstOptimal
			);

			vk::BufferImageCopy bufferImageCopy {
				0u,
				0u,
				0u,
				vk::ImageSubresourceLayers {
				    vk::ImageAspectFlagBits::eColor,
				    0u,
				    0u,
				    1u,
				},
				vk::Offset3D { 0, 0, 0 },
				vk::Extent3D { texture.width, texture.height, 1u },
			};

			cmd.copyBufferToImage(
			    staging_buffer,
			    texture.image,
			    vk::ImageLayout::eTransferDstOptimal,
			    1u,
			    &bufferImageCopy
			);

			/////////////////////////////////////////////////////////////////////////////////
			/// Create texture mipmaps
			vk::ImageMemoryBarrier barrier {
				{},
				{},
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eUndefined,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				texture.image,

				vk::ImageSubresourceRange {
				    vk::ImageAspectFlagBits::eColor,
				    0u,
				    1u,
				    0u,
				    1u,
				},
			};

			i32 mip_width = width;
			i32 mip_height = height;

			for (u32 i = 1; i < texture.mip_levels; i++)
			{
				// @todo hacked
				texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
				texture.transition_layout(
				    device,
				    cmd,
				    i - 1u,
				    1u,
				    1u,
				    vk::ImageLayout::eTransferSrcOptimal
				);

				blit_iamge(cmd, texture.image, i, mip_width, mip_height);

				texture.transition_layout(
				    device,
				    cmd,
				    i - 1u,
				    1u,
				    1u,
				    vk::ImageLayout::eShaderReadOnlyOptimal
				);
			}

			// @todo hacked
			texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
			texture.transition_layout(
			    device,
			    cmd,
			    texture.mip_levels - 1ul,
			    1u,
			    1u,
			    vk::ImageLayout::eShaderReadOnlyOptimal
			);
		});

		// Copy buffer to image is executed, delete the staging buffer
		device->allocator.destroyBuffer(staging_buffer, staging_buffer);

		/////////////////////////////////////////////////////////////////////////////////
		// Create image views
		{
			vk::ImageViewCreateInfo image_view_info {
				{},
				texture.image,
				vk::ImageViewType::e2D,
				vk::Format::eR8G8B8A8Srgb,
				vk::ComponentMapping {
				    vk::ComponentSwizzle::eIdentity,
				    vk::ComponentSwizzle::eIdentity,
				    vk::ComponentSwizzle::eIdentity,
				    vk::ComponentSwizzle::eIdentity,
				},
				vk::ImageSubresourceRange {
				    vk::ImageAspectFlagBits::eColor,
				    0u,
				    texture.mip_levels,
				    0u,
				    1u,
				},
			};

			texture.image_view = device->logical.createImageView(image_view_info, nullptr);
		}

		/////////////////////////////////////////////////////////////////////////////////
		// Create image samplers
		{
			vk::SamplerCreateInfo sampler_info {
				{},
				vk::Filter::eLinear,
				vk::Filter::eLinear,
				vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				0.0f,
				VK_FALSE,
				{},
				VK_FALSE,
				vk::CompareOp::eAlways,
				0.0f,
				static_cast<float>(texture.mip_levels),
				vk::BorderColor::eIntOpaqueBlack,
				VK_FALSE,
			};

			/// @todo Should we separate samplers and textures?
			texture.sampler = device->logical.createSampler(sampler_info, nullptr);
		}

		texture.descriptor_info = {
			texture.sampler,
			texture.image_view,
			vk::ImageLayout::eShaderReadOnlyOptimal,
		};
	}

	return texture;
}

Texture TextureLoader::load_from_ktx(
    c_str name,
    c_str uri,
    Texture::Type type,
    Buffer* staging_buffer,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	KtxLoader loader(device, staging_buffer);
	return loader.load(name, uri, type, layout);
}

void TextureLoader::blit_iamge(
    vk::CommandBuffer cmd,
    AllocatedImage image,
    u32 mip_index,
    i32& mip_width,
    i32& mip_height
)
{
	vk::ImageBlit blit {
		vk::ImageSubresourceLayers {
		    vk::ImageAspectFlagBits::eColor,
		    mip_index - 1u,
		    0u,
		    1u,
		},
		{
		    vk::Offset3D { 0, 0, 0 },
		    vk::Offset3D { mip_width, mip_height, 1 },
		},
		vk::ImageSubresourceLayers {
		    vk::ImageAspectFlagBits::eColor,
		    mip_index,
		    0u,
		    1u,
		},
		{
		    vk::Offset3D { 0, 0, 0 },
		    vk::Offset3D { mip_width > 1 ? mip_width / 2 : 1,
		                   mip_height > 1 ? mip_height / 2 : 1,
		                   1 },
		},
	};

	cmd.blitImage(
	    image,
	    vk::ImageLayout::eTransferSrcOptimal,
	    image,
	    vk::ImageLayout::eTransferDstOptimal,
	    1u,
	    &blit,
	    vk::Filter::eLinear
	);

	if (mip_width > 1)
		mip_width /= 2;

	if (mip_height > 1)
		mip_height /= 2;
}

} // namespace BINDLESSVK_NAMESPACE

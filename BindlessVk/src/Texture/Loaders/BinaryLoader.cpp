#include "BindlessVk/Texture/Loaders/BinaryLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

BinaryLoader::BinaryLoader(VkContext const *const vk_context, Buffer *staging_buffer)
    : vk_context(vk_context)
    , staging_buffer(staging_buffer)
{
}

Texture BinaryLoader::load(
    u8 const *const pixels,
    u32 width,
    u32 height,
    vk::DeviceSize size,
    Texture::Type type,
    vk::ImageLayout final_layout,
    str_view const debug_name
)
{
	texture.vk_context = vk_context;
	texture.size = { width, height };
	texture.format = vk::Format::eR8G8B8A8Srgb;
	texture.mip_levels = std::floor(std::log2(std::max(width, height))) + 1;
	texture.device_size = size;
	texture.debug_name = debug_name;

	create_image();
	create_image_view();
	create_sampler();

	stage_texture_data(pixels, size);
	write_texture_data_to_gpu();

	return std::move(texture);
}

void BinaryLoader::create_image()
{
	auto const [width, height] = texture.size;

	texture.image = vk_context->get_allocator().createImage(
	    vk::ImageCreateInfo {
	        {},
	        vk::ImageType::e2D,
	        vk::Format::eR8G8B8A8Srgb,
	        vk::Extent3D {
	            width,
	            height,
	            1u,
	        },
	        texture.mip_levels,
	        1u,
	        vk::SampleCountFlagBits::e1,
	        vk::ImageTiling::eOptimal,
	        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
	            vk::ImageUsageFlagBits::eSampled,
	        vk::SharingMode::eExclusive,
	        0u,
	        nullptr,
	        vk::ImageLayout::eUndefined,
	    },

	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eGpuOnly,
	        vk::MemoryPropertyFlagBits::eDeviceLocal,
	    }
	);
}

void BinaryLoader::create_image_view()
{
	texture.image_view = vk_context->get_device().createImageView(vk::ImageViewCreateInfo {
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
	});

	texture.descriptor_info.imageView = texture.image_view;
}

void BinaryLoader::create_sampler()
{
	texture.sampler = vk_context->get_device().createSampler(vk::SamplerCreateInfo {
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
	    static_cast<f32>(texture.mip_levels),
	    vk::BorderColor::eIntOpaqueBlack,
	    VK_FALSE,
	});

	texture.descriptor_info.sampler = texture.sampler;
}

void BinaryLoader::stage_texture_data(u8 const *const pixels, vk::DeviceSize size)
{
	memcpy(staging_buffer->map_block(0), pixels, size);
	staging_buffer->unmap();
}

void BinaryLoader::write_texture_data_to_gpu()
{
	vk_context->immediate_submit([&](vk::CommandBuffer cmd) {
		auto const [width, height] = texture.size;

		texture.transition_layout(
		    vk_context,
		    cmd,
		    0u,
		    texture.mip_levels,
		    1u,
		    vk::ImageLayout::eTransferDstOptimal
		);


		cmd.copyBufferToImage(
		    *staging_buffer->get_buffer(),
		    texture.image,
		    vk::ImageLayout::eTransferDstOptimal,
		    vk::BufferImageCopy {
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
		        vk::Extent3D { width, height, 1u },
		    }
		);

		create_mipmaps(cmd);

		// @todo hacked
		texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
		texture.transition_layout(
		    vk_context,
		    cmd,
		    texture.mip_levels - 1ul,
		    1u,
		    1u,
		    vk::ImageLayout::eShaderReadOnlyOptimal
		);
	});

	texture.descriptor_info.imageLayout = texture.current_layout;
}

void BinaryLoader::create_mipmaps(vk::CommandBuffer cmd)
{
	auto [mip_width, mip_height] = texture.size;

	for (u32 i = 1u; i < texture.mip_levels; ++i)
	{
		// @todo hacked
		texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
		texture.transition_layout(
		    vk_context,
		    cmd,
		    i - 1u,
		    1u,
		    1u,
		    vk::ImageLayout::eTransferSrcOptimal
		);

		texture.blit(cmd, i, { mip_width, mip_height });

		mip_width = mip_width > 1 ? mip_width / 2.0 : mip_width;
		mip_height = mip_height > 1 ? mip_height / 2.0 : mip_height;

		texture.transition_layout(
		    vk_context,
		    cmd,
		    i - 1u,
		    1u,
		    1u,
		    vk::ImageLayout::eShaderReadOnlyOptimal
		);
	}
}

} // namespace BINDLESSVK_NAMESPACE

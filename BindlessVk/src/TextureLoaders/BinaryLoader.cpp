#include "BindlessVk/TextureLoaders/BinaryLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

BinaryLoader::BinaryLoader(Device* device, Buffer* staging_buffer)
    : device(device)
    , staging_buffer(staging_buffer)
{
}

Texture BinaryLoader::load(
    c_str name,
    u8* pixels,
    u32 width,
    u32 height,
    vk::DeviceSize size,
    Texture::Type type,
    vk::ImageLayout final_layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	texture = Texture {
		name,
		{},
		width,
		height,
		vk::Format::eR8G8B8A8Srgb,
		static_cast<u32>(std::floor(std::log2(std::max(width, height))) + 1),
		size,
		{},
		{},
		vk::ImageLayout::eUndefined,
		{},
	};

	create_image();
	create_image_view();
	create_sampler();

	stage_texture_data(pixels, size);
	write_texture_data_to_gpu();

	return texture;
}

void BinaryLoader::create_image()
{
	texture.image = device->allocator.createImage(
	    vk::ImageCreateInfo {
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
	texture.image_view = device->logical.createImageView(
	    vk::ImageViewCreateInfo {
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
	    },
	    nullptr
	);

	texture.descriptor_info.imageView = texture.image_view;
}

void BinaryLoader::create_sampler()
{
	texture.sampler = device->logical.createSampler(
	    vk::SamplerCreateInfo {
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
	    },
	    nullptr
	);

	texture.descriptor_info.sampler = texture.sampler;
}

void BinaryLoader::stage_texture_data(u8* pixels, vk::DeviceSize size)
{
	memcpy(staging_buffer->map_block(0), pixels, size);
	staging_buffer->unmap();
}

void BinaryLoader::write_texture_data_to_gpu()
{
	device->immediate_submit([&](vk::CommandBuffer cmd) {
		texture.transition_layout(
		    device,
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
		        vk::Extent3D { texture.width, texture.height, 1u },
		    }
		);

		create_mipmaps(cmd);

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

	texture.descriptor_info.imageLayout = texture.current_layout;
}

void BinaryLoader::create_mipmaps(vk::CommandBuffer cmd)
{
	auto mip_size = std::make_pair(texture.width, texture.height);

	for (u32 i = 1u; i < texture.mip_levels; ++i)
	{
		// @todo hacked
		texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
		texture
		    .transition_layout(device, cmd, i - 1u, 1u, 1u, vk::ImageLayout::eTransferSrcOptimal);

		mip_size = texture.blit(cmd, i, mip_size);

		texture.transition_layout(
		    device,
		    cmd,
		    i - 1u,
		    1u,
		    1u,
		    vk::ImageLayout::eShaderReadOnlyOptimal
		);
	}
}

} // namespace BINDLESSVK_NAMESPACE

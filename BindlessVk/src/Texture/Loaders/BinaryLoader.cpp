#include "BindlessVk/Texture/Loaders/BinaryLoader.hpp"

#include "Amender/Amender.hpp"

namespace BINDLESSVK_NAMESPACE {

BinaryLoader::BinaryLoader(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    Buffer *const staging_buffer
)
    : device(vk_context->get_device())
    , memory_allocator(memory_allocator)
    , staging_buffer(staging_buffer)
{
	ScopeProfiler _;

	texture.device = device;
	texture.memory_allocator = memory_allocator;
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
	ScopeProfiler _;

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
	ScopeProfiler _;

	auto const [width, height] = texture.size;

	texture.image = Image {
		memory_allocator,

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
		},
	};
}

void BinaryLoader::create_image_view()
{
	ScopeProfiler _;

	texture.image_view = device->vk().createImageView(vk::ImageViewCreateInfo {
	    {},
	    texture.image.vk(),
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
	ScopeProfiler _;

	texture.sampler = device->vk().createSampler(vk::SamplerCreateInfo {
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
	ScopeProfiler _;

	memcpy(staging_buffer->map_block(0), pixels, size);
	staging_buffer->unmap();
}

void BinaryLoader::write_texture_data_to_gpu()
{
	ScopeProfiler _;

	device->immediate_submit([&](vk::CommandBuffer cmd) {
		auto const [width, height] = texture.size;

		texture.transition_layout(
		    cmd,
		    0u,
		    texture.mip_levels,
		    1u,
		    vk::ImageLayout::eTransferDstOptimal
		);


		cmd.copyBufferToImage(
		    *staging_buffer->vk(),
		    texture.image.vk(),
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
	ScopeProfiler _;

	auto [mip_width, mip_height] = texture.size;

	for (u32 i = 1u; i < texture.mip_levels; ++i)
	{
		// @todo hacked
		texture.current_layout = vk::ImageLayout::eTransferDstOptimal;
		texture.transition_layout(cmd, i - 1u, 1u, 1u, vk::ImageLayout::eTransferSrcOptimal);

		texture.blit(cmd, i, { mip_width, mip_height });

		mip_width = mip_width > 1 ? mip_width / 2.0 : mip_width;
		mip_height = mip_height > 1 ? mip_height / 2.0 : mip_height;

		texture.transition_layout(cmd, i - 1u, 1u, 1u, vk::ImageLayout::eShaderReadOnlyOptimal);
	}
}

} // namespace BINDLESSVK_NAMESPACE

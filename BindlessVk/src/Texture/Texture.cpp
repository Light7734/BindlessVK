#include "BindlessVk/Texture/Texture.hpp"

#include "Amender/Logger.hpp"

namespace BINDLESSVK_NAMESPACE {

Texture::~Texture()
{
	if (!device)
		return;

	device->vk().destroyImageView(image_view);
	device->vk().destroySampler(sampler);
}

void Texture::transition_layout(
    vk::CommandBuffer const cmd,
    u32 const base_mip_level,
    u32 const level_count,
    u32 const layer_count,
    vk::ImageLayout const new_layout
)
{
	// Memory barrier
	auto image_memory_barrier = vk::ImageMemoryBarrier {
		{},
		{},
		current_layout,
		new_layout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image.vk(),
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eColor,
		    base_mip_level,
		    level_count,
		    0u,
		    layer_count,
		},
	};

	/* Specify the source & destination stages and access masks... */
	auto src_stage = vk::PipelineStageFlags {};
	auto dst_stage = vk::PipelineStageFlags {};

	// Undefined -> TRANSFER DST
	if (current_layout == vk::ImageLayout::eUndefined &&
	    new_layout == vk::ImageLayout::eTransferDstOptimal)
	{
		image_memory_barrier.srcAccessMask = {};
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}

	// TRANSFER DST -> TRANSFER SRC
	else if (current_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eTransferSrcOptimal)
	{
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}

	// TRANSFER SRC -> SHADER READ
	else if (current_layout == vk::ImageLayout::eTransferSrcOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}

	// TRANSFER DST -> SHADER READ
	else if (current_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}

	else
	{
		log_err(
		    "Texture transition layout to/from unexpected layout(s) \n {} -> {}",
		    static_cast<i32>(current_layout),
		    static_cast<i32>(new_layout)
		);
	}

	// Execute pipeline barrier
	cmd.pipelineBarrier(src_stage, dst_stage, {}, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
	current_layout = new_layout;
}

void Texture::blit(vk::CommandBuffer const cmd, u32 const mip_index, pair<i32, i32> const mip_size)
{
	auto const [mip_width, mip_height] = mip_size;

	cmd.blitImage(
	    image.vk(),
	    vk::ImageLayout::eTransferSrcOptimal,
	    image.vk(),
	    vk::ImageLayout::eTransferDstOptimal,
	    vk::ImageBlit {
	        vk::ImageSubresourceLayers {
	            vk::ImageAspectFlagBits::eColor,
	            mip_index - 1u,
	            0u,
	            1u,
	        },
	        arr<vk::Offset3D, 2> {
	            vk::Offset3D { 0, 0, 0 },
	            vk::Offset3D { mip_width, mip_height, 1 },
	        },
	        vk::ImageSubresourceLayers {
	            vk::ImageAspectFlagBits::eColor,
	            mip_index,
	            0u,
	            1u,
	        },
	        arr<vk::Offset3D, 2> {
	            vk::Offset3D { 0, 0, 0 },
	            vk::Offset3D {
	                std::max(mip_width / 2, 1),
	                std::max(mip_height / 2, 1),
	                1,
	            },
	        },
	    },
	    vk::Filter::eLinear
	);
}

} // namespace BINDLESSVK_NAMESPACE

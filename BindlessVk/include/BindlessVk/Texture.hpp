#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Texture
{
	enum class Type : u8
	{
		e2D,
		e2DArray,
		eCubeMap
	};


	c_str name;

	// @todo: should we delete sampler,image_view,current_layout because of this?
	vk::DescriptorImageInfo descriptor_info;

	u32 width;
	u32 height;
	vk::Format format;
	u32 channels;
	u32 mip_levels;
	vk::DeviceSize size;

	vk::Sampler sampler;
	vk::ImageView image_view;
	vk::ImageLayout current_layout;

	AllocatedImage image;

	void transition_layout(
	    Device* device,
	    vk::CommandBuffer cmd,
	    u32 base_mip_level,
	    u32 level_count,
	    u32 layer_count,
	    vk::ImageLayout new_layout
	)
	{
		// Memory barrier
		vk::ImageMemoryBarrier imageMemBarrier {
			{},
			{},
			current_layout,
			new_layout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			vk::ImageSubresourceRange {
			    vk::ImageAspectFlagBits::eColor,
			    base_mip_level,
			    level_count,
			    0u,
			    layer_count,
			},
		};
		/* Specify the source & destination stages and access masks... */
		vk::PipelineStageFlags srcStage;
		vk::PipelineStageFlags dstStage;

		// Undefined -> TRANSFER DST
		if (current_layout == vk::ImageLayout::eUndefined
		    && new_layout == vk::ImageLayout::eTransferDstOptimal)
		{
			imageMemBarrier.srcAccessMask = {};
			imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
			dstStage = vk::PipelineStageFlagBits::eTransfer;
		}

		// TRANSFER DST -> TRANSFER SRC
		else if (current_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eTransferSrcOptimal)
		{
			imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			srcStage = vk::PipelineStageFlagBits::eTransfer;
			dstStage = vk::PipelineStageFlagBits::eTransfer;
		}

		// TRANSFER SRC -> SHADER READ
		else if (current_layout == vk::ImageLayout::eTransferSrcOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			srcStage = vk::PipelineStageFlagBits::eTransfer;
			dstStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		//
		// TRANSFER DST -> SHADER READ
		else if (current_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			srcStage = vk::PipelineStageFlagBits::eTransfer;
			dstStage = vk::PipelineStageFlagBits::eFragmentShader;
		}

		else
		{
			device->log(
			    LogLvl::eError,
			    "Texture transition layout to/from unexpected layout(s) \n {} -> {}",
			    static_cast<i32>(current_layout),
			    static_cast<i32>(new_layout)
			);
		}

		// Execute pipeline barrier
		cmd.pipelineBarrier(srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &imageMemBarrier);
		current_layout = new_layout;
	}
};

} // namespace BINDLESSVK_NAMESPACE

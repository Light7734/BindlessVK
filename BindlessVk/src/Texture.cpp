#include "BindlessVk/Texture.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

void TextureSystem::init(Device* device)
{
	this->device = device;

	BVK_ASSERT(
	    !(device->physical.getFormatProperties(vk::Format::eR8G8B8A8Srgb).optimalTilingFeatures
	      & vk::FormatFeatureFlagBits::eSampledImageFilterLinear),
	    "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);

	vk::CommandPoolCreateInfo commnad_pool_create_info {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		device->graphics_queue_index,
	};
}

Texture* TextureSystem::create_from_buffer(
    const std::string& name,
    u8* pixels,
    i32 width,
    i32 height,
    vk::DeviceSize size,
    Texture::Type type,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	Texture& texture = textures[HashStr(name.c_str())];
	texture = {
		.descriptor_info = {},
		.width = static_cast<u32>(width),
		.height = static_cast<u32>(height),
		.format = vk::Format::eR8G8B8A8Srgb,
		.mip_levels = static_cast<u32>(std::floor(std::log2(std::max(width, height))) + 1),
		.size = size,
		.sampler = {},
		.image_view = {},
		.layout = layout,
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
			transition_layout(
			    texture,
			    cmd,
			    0u,
			    texture.mip_levels,
			    1u,
			    vk::ImageLayout::eUndefined,
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
				transition_layout(
				    texture,
				    cmd,
				    i - 1u,
				    1u,
				    1u,
				    vk::ImageLayout::eTransferDstOptimal,
				    vk::ImageLayout::eTransferSrcOptimal
				);

				blit_iamge(cmd, texture.image, i, mip_width, mip_height);

				transition_layout(
				    texture,
				    cmd,
				    i - 1u,
				    1u,
				    1u,
				    vk::ImageLayout::eTransferSrcOptimal,
				    vk::ImageLayout::eShaderReadOnlyOptimal
				);
			}

			transition_layout(
			    texture,
			    cmd,
			    texture.mip_levels - 1ul,
			    1u,
			    1u,
			    vk::ImageLayout::eTransferDstOptimal,
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

	return &textures[HashStr(name.c_str())];
}

Texture* TextureSystem::create_from_gltf(
    const tinygltf::Image& image,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	return create_from_buffer(
	    image.uri,
	    &const_cast<tinygltf::Image&>(image).image[0],
	    image.width,
	    image.height,
	    image.image.size(),
	    Texture::Type::e2D,
	    layout
	);
}

Texture* TextureSystem::create_from_ktx(
    const std::string& name,
    const std::string& uri,
    Texture::Type type,
    vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	ktxTexture* texture_ktx;
	BVK_ASSERT(
	    ktxTexture_CreateFromNamedFile(
	        uri.c_str(),
	        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
	        &texture_ktx
	    ),
	    "Failed to load ktx file"
	);

	textures[HashStr(name.c_str())] = {
		.descriptor_info = {},
		.width = texture_ktx->baseWidth,
		.height = texture_ktx->baseHeight,
		.format = vk::Format::eB8G8R8A8Srgb,
		.mip_levels = texture_ktx->numLevels,
		.size = ktxTexture_GetSize(texture_ktx),
		.sampler = {},
		.image_view = {},
		.layout = layout,
		.image = {},
	};
	u8* data = ktxTexture_GetData(texture_ktx);
	Texture& texture = textures[HashStr(name.c_str())];

	vk::BufferCreateInfo staging_buffer_info {
		{},
		texture.size,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eExclusive,
	};

	AllocatedBuffer staging_buffer;
	vma::AllocationCreateInfo buffer_allocate_info(
	    {},
	    vma::MemoryUsage::eCpuOnly,
	    { vk::MemoryPropertyFlagBits::eHostCached }
	);

	staging_buffer = device->allocator.createBuffer(staging_buffer_info, buffer_allocate_info);

	// Copy data to staging buffer
	memcpy(device->allocator.mapMemory(staging_buffer), data, texture.size);
	device->allocator.unmapMemory(staging_buffer);

	std::vector<vk::BufferImageCopy> bufferCopies;

	BVK_ASSERT(
	    (type != Texture::Type::eCubeMap),
	    "Create From KTX Only supports cubemaps for the time being..."
	)

	for (u32 face = 0; face < 6; face++)
	{
		for (u32 level = 0u; level < texture.mip_levels; level++)
		{
			size_t offset;
			BVK_ASSERT(
			    ktxTexture_GetImageOffset(texture_ktx, level, 0u, face, &offset),
			    "Failed to get ktx image offset"
			);

			bufferCopies.push_back({
			    offset,
			    {},
			    {},
			    {
			        vk::ImageAspectFlagBits::eColor,
			        level,
			        face,
			        1u,
			    },
			    {},
			    {
			        texture.width >> level,
			        texture.height >> level,
			        1u,
			    },
			});
		}
	}

	vk::ImageCreateInfo image_create_info {
		vk::ImageCreateFlagBits::eCubeCompatible,
		vk::ImageType::e2D,
		texture.format,
		vk::Extent3D {
		    texture.width,
		    texture.height,
		    1u,
		},
		texture.mip_levels,
		6u,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
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

	device->immediate_submit([&](vk::CommandBuffer&& cmd) {
		transition_layout(
		    texture,
		    cmd,
		    0u,
		    texture.mip_levels,
		    6u,
		    vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eTransferDstOptimal
		);

		cmd.copyBufferToImage(
		    staging_buffer,
		    texture.image,
		    vk::ImageLayout::eTransferDstOptimal,
		    static_cast<u32>(bufferCopies.size()),
		    bufferCopies.data()
		);

		transition_layout(
		    texture,
		    cmd,
		    0u,
		    texture.mip_levels,
		    6u,
		    vk::ImageLayout::eTransferDstOptimal,
		    texture.layout
		);
	});

	/////////////////////////////////////////////////////////////////////////////////
	// Create image views
	{
		vk::ImageViewCreateInfo image_view_info {
			{},
			texture.image,
			vk::ImageViewType::eCube,
			texture.format,
			vk::ComponentMapping {
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			},

			/* subresourceRange */
			vk::ImageSubresourceRange {
			    vk::ImageAspectFlagBits::eColor,
			    0u,
			    texture.mip_levels,
			    0u,
			    6u,
			},
		};

		texture.image_view = device->logical.createImageView(image_view_info, nullptr);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// Create image samplers
	{
		vk::SamplerCreateInfo samplerCreateInfo {
			{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			0.0f,
			// #TODO ANISOTROPY
			VK_FALSE,
			{},
			VK_FALSE,
			vk::CompareOp::eNever,
			0.0f,
			static_cast<float>(texture.mip_levels),
			vk::BorderColor::eFloatOpaqueWhite,
			VK_FALSE,
		};

		/// @todo Should we separate samplers and textures?
		texture.sampler = device->logical.createSampler(samplerCreateInfo, nullptr);
	}

	texture.descriptor_info = {
		texture.sampler,
		texture.image_view,
		texture.layout,
	};

	/////////////////////////////////////////////////////////////////////////////////
	/// Cleanup
	device->allocator.destroyBuffer(staging_buffer, staging_buffer);
	ktxTexture_Destroy(texture_ktx);

	return &textures[HashStr(name.c_str())];
}

void TextureSystem::blit_iamge(
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

void TextureSystem::reset()
{
	for (auto& [key, value] : textures)
	{
		device->logical.destroySampler(value.sampler, nullptr);
		device->logical.destroyImageView(value.image_view, nullptr);
		device->allocator.destroyImage(value.image, value.image);
	}
}

void TextureSystem::transition_layout(
    Texture& texture,
    vk::CommandBuffer cmdBuffer,
    u32 baseMipLevel,
    u32 levelCount,
    u32 layerCount,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
)
{
	// Memory barrier
	vk::ImageMemoryBarrier imageMemBarrier {
		{},
		{},
		oldLayout,
		newLayout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		texture.image,
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eColor,
		    baseMipLevel,
		    levelCount,
		    0u,
		    layerCount,
		},
	};
	/* Specify the source & destination stages and access masks... */
	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;

	// Undefined -> TRANSFER DST
	if (oldLayout == vk::ImageLayout::eUndefined
	    && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		imageMemBarrier.srcAccessMask = {};
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}

	// TRANSFER DST -> TRANSFER SRC
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}

	// TRANSFER SRC -> SHADER READ
	else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	//
	// TRANSFER DST -> SHADER READ
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}

	else
	{
		BVK_LOG(
		    LogLvl::eError,
		    "Texture transition layout to/from unexpected layout(s) \n {} -> {}",
		    (i32)oldLayout,
		    (i32)newLayout
		);
	}

	// Execute pipeline barrier
	cmdBuffer.pipelineBarrier(srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &imageMemBarrier);
}

} // namespace BINDLESSVK_NAMESPACE

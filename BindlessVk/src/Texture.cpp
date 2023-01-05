#include "BindlessVk/Texture.hpp"

#include <AssetParser.hpp>
#include <TextureAsset.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

void TextureSystem::init(Device* device)
{
	m_device = device;
	BVK_ASSERT(
	  !(m_device->physical.getFormatProperties(vk::Format::eR8G8B8A8Srgb)
	      .optimalTilingFeatures
	    & vk::FormatFeatureFlagBits::eSampledImageFilterLinear),
	  "Texture image format(eR8G8B8A8Srgb) does not support linear blitting"
	);

	vk::CommandPoolCreateInfo commnad_pool_create_info {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
		m_device->graphics_queue_index,                     // queueFamilyIndex
	};
}

Texture* TextureSystem::create_from_buffer(
  const std::string& name,
  uint8_t* pixels,
  int width,
  int height,
  vk::DeviceSize size,
  Texture::Type type,
  vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	m_textures[HashStr(name.c_str())] = {
		.descriptor_info = {},
		.width           = static_cast<uint32_t>(width),
		.height          = static_cast<uint32_t>(height),
		.format          = vk::Format::eR8G8B8A8Srgb,
		.mip_levels      = static_cast<uint32_t>(
      std::floor(std::log2(std::max(width, height))) + 1
    ),
		.size       = size,
		.sampler    = {},
		.image_view = {},
		.layout     = layout,
		.image      = {},
	};

	Texture& texture = m_textures[HashStr(name.c_str())];

	/////////////////////////////////////////////////////////////////////////////////
	// Create the image and staging buffer
	{
		// Create vulkan image
		vk::ImageCreateInfo image_create_info {
			{},                        // flags
			vk::ImageType::e2D,        // imageType
			vk::Format::eR8G8B8A8Srgb, // format
			/* extent */
			vk::Extent3D {
			  texture.width,  // width
			  texture.height, // height
			  1u,             // depth
			},
			texture.mip_levels,          // mipLevels
			1u,                          // arrayLayers
			vk::SampleCountFlagBits::e1, // samples
			vk::ImageTiling::eOptimal,   // tiling
			vk::ImageUsageFlagBits::eTransferDst
			  | vk::ImageUsageFlagBits::eTransferSrc
			  | vk::ImageUsageFlagBits::eSampled, // usage
			vk::SharingMode::eExclusive,          // sharingMode
			0u,                                   // queueFamilyIndexCount
			nullptr,                              // queueFamilyIndices
			vk::ImageLayout::eUndefined,          // initialLayout
		};

		vma::AllocationCreateInfo image_allocate_info(
		  {},                                      // flags
		  vma::MemoryUsage::eGpuOnly,              // usage
		  vk::MemoryPropertyFlagBits::eDeviceLocal // requiredFlags
		);

		texture.image = m_device->allocator.createImage(
		  image_create_info,
		  image_allocate_info
		);

		// Create staging buffer
		vk::BufferCreateInfo staging_buffer_create_info {
			{},                                    // flags
			size,                                  // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		vma::AllocationCreateInfo buffer_allocate_info(
		  {},                                         // flags
		  vma::MemoryUsage::eCpuOnly,                 //  usage
		  { vk::MemoryPropertyFlagBits::eHostCached } // requiredFlags
		);

		AllocatedBuffer staging_buffer;
		staging_buffer = m_device->allocator.createBuffer(
		  staging_buffer_create_info,
		  buffer_allocate_info
		);

		// Copy data to staging buffer
		memcpy(m_device->allocator.mapMemory(staging_buffer), pixels, size);
		m_device->allocator.unmapMemory(staging_buffer);

		m_device->immediate_submit([&](vk::CommandBuffer cmd) {
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
				0u, // bufferOffset
				0u, // bufferRowLength
				0u, // bufferImageHeight

				/* imageSubresource */
				vk::ImageSubresourceLayers {
				  vk::ImageAspectFlagBits::eColor, // aspectMask
				  0u,                              // mipLevel
				  0u,                              // baseArrayLayer
				  1u,                              // layerCount
				},
				vk::Offset3D { 0, 0, 0 },                           // imageOffset
				vk::Extent3D { texture.width, texture.height, 1u }, // imageExtent
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
				{},                          // srcAccessmask
				{},                          // dstAccessMask
				vk::ImageLayout::eUndefined, // oldLayout
				vk::ImageLayout::eUndefined, // newLayout
				VK_QUEUE_FAMILY_IGNORED,     // srcQueueFamilyIndex
				VK_QUEUE_FAMILY_IGNORED,     // dstQueueFamilyIndex
				texture.image,               // image

				vk::ImageSubresourceRange {
				  vk::ImageAspectFlagBits::eColor, // aspectMask
				  0u,                              // baseMipLevel
				  1u,                              // levelCount
				  0u,                              // baseArrayLayer
				  1u,                              // layerCount
				},
			};

			int32_t mip_width  = width;
			int32_t mip_height = height;

			for (uint32_t i = 1; i < texture.mip_levels; i++)
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
		m_device->allocator.destroyBuffer(staging_buffer, staging_buffer);

		/////////////////////////////////////////////////////////////////////////////////
		// Create image views
		{
			vk::ImageViewCreateInfo image_view_info {
				{},                        // flags
				texture.image,             // image
				vk::ImageViewType::e2D,    // viewType
				vk::Format::eR8G8B8A8Srgb, // format

				/* components */
				vk::ComponentMapping {
				  // Don't swizzle the colors around...
				  vk::ComponentSwizzle::eIdentity, // r
				  vk::ComponentSwizzle::eIdentity, // g
				  vk::ComponentSwizzle::eIdentity, // b
				  vk::ComponentSwizzle::eIdentity, // a
				},

				/* subresourceRange */
				vk::ImageSubresourceRange {
				  vk::ImageAspectFlagBits::eColor, // aspectMask
				  0u,                              // baseMipLevel
				  texture.mip_levels,              // levelCount
				  0u,                              // baseArrayLayer
				  1u,                              // layerCount
				},
			};

			texture.image_view = m_device->logical.createImageView(
			  image_view_info,
			  nullptr
			);
		}
		/////////////////////////////////////////////////////////////////////////////////
		// Create image samplers
		{
			vk::SamplerCreateInfo sampler_info {
				{},
				vk::Filter::eLinear,             // magFilter
				vk::Filter::eLinear,             // minFilter
				vk::SamplerMipmapMode::eLinear,  // mipmapMode
				vk::SamplerAddressMode::eRepeat, // addressModeU
				vk::SamplerAddressMode::eRepeat, // addressModeV
				vk::SamplerAddressMode::eRepeat, // addressModeW
				0.0f,                            // mipLodBias

				// #TODO ANISOTROPY
				VK_FALSE, // anisotropyEnable
				{},       // maxAnisotropy

				VK_FALSE,                               // compareEnable
				vk::CompareOp::eAlways,                 // compareOp
				0.0f,                                   // minLod
				static_cast<float>(texture.mip_levels), // maxLod
				vk::BorderColor::eIntOpaqueBlack,       // borderColor
				VK_FALSE,                               // unnormalizedCoordinates
			};

			/// @todo Should we separate samplers and textures?
			texture.sampler = m_device->logical.createSampler(sampler_info, nullptr);
		}

		texture.descriptor_info = {
			texture.sampler,                         // sampler
			texture.image_view,                      // imageView
			vk::ImageLayout::eShaderReadOnlyOptimal, // imageLayout
		};
	}

	return &m_textures[HashStr(name.c_str())];
}

Texture* TextureSystem::create_from_gltf(
  struct tinygltf::Image* image,
  vk::ImageLayout layout /* = vk::ImageLayout::eShaderReadOnlyOptimal */
)
{
	return create_from_buffer(
	  image->uri,
	  &image->image[0],
	  image->width,
	  image->height,
	  image->image.size(),
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

	m_textures[HashStr(name.c_str())] = {
		.descriptor_info = {},
		.width           = texture_ktx->baseWidth,
		.height          = texture_ktx->baseHeight,
		.format          = vk::Format::eB8G8R8A8Srgb,
		.mip_levels      = texture_ktx->numLevels,
		.size            = ktxTexture_GetSize(texture_ktx),
		.sampler         = {},
		.image_view      = {},
		.layout          = layout,
		.image           = {},
	};
	uint8_t* data    = ktxTexture_GetData(texture_ktx);
	Texture& texture = m_textures[HashStr(name.c_str())];

	// Create staging buffer
	vk::BufferCreateInfo staging_buffer_info {
		{},                                    // flags
		texture.size,                          // size
		vk::BufferUsageFlagBits::eTransferSrc, // usage
		vk::SharingMode::eExclusive,           // sharingMode
	};

	AllocatedBuffer staging_buffer;
	vma::AllocationCreateInfo buffer_allocate_info(
	  {},
	  vma::MemoryUsage::eCpuOnly,
	  { vk::MemoryPropertyFlagBits::eHostCached }
	);

	staging_buffer = m_device->allocator.createBuffer(
	  staging_buffer_info,
	  buffer_allocate_info
	);

	// Copy data to staging buffer
	memcpy(m_device->allocator.mapMemory(staging_buffer), data, texture.size);
	m_device->allocator.unmapMemory(staging_buffer);

	std::vector<vk::BufferImageCopy> bufferCopies;

	BVK_ASSERT(
	  (type != Texture::Type::eCubeMap),
	  "Create From KTX Only supports cubemaps for the time being..."
	)

	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0u; level < texture.mip_levels; level++)
		{
			size_t offset;
			BVK_ASSERT(
			  ktxTexture_GetImageOffset(texture_ktx, level, 0u, face, &offset),
			  "Failed to get ktx image offset"
			);

			bufferCopies.push_back({
			  offset, // bufferOffset
			  {},     // bufferRowLength
			  {},     // bufferImageHeight

			  /* imageSubresource */
			  {
			    vk::ImageAspectFlagBits::eColor, // aspectMask
			    level,                           // mipLevel
			    face,                            // baseArrayLayer
			    1u,                              // layerCount
			  },

			  /* imageOffset */
			  {},

			  /* imageExtent */
			  {
			    texture.width >> level,  // width
			    texture.height >> level, // height
			    1u,                      // depth
			  },
			});
		}
	}

	// Create vulkan image
	vk::ImageCreateInfo image_create_info {
		vk::ImageCreateFlagBits::eCubeCompatible, // flags
		vk::ImageType::e2D,                       // imageType
		texture.format,                           // format

		/* extent */
		vk::Extent3D {
		  texture.width,  // width
		  texture.height, // height
		  1u,             // depth
		},
		texture.mip_levels,          // mipLevels
		6u,                          // arrayLayers
		vk::SampleCountFlagBits::e1, // samples
		vk::ImageTiling::eOptimal,   // tiling
		vk::ImageUsageFlagBits::eTransferDst
		  | vk::ImageUsageFlagBits::eSampled, // usage
		vk::SharingMode::eExclusive,          // sharingMode
		0u,                                   // queueFamilyIndexCount
		nullptr,                              // queueFamilyIndices
		vk::ImageLayout::eUndefined,          // initialLayout
	};

	vma::AllocationCreateInfo image_allocate_info(
	  {},
	  vma::MemoryUsage::eGpuOnly,
	  vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	texture.image = m_device->allocator.createImage(
	  image_create_info,
	  image_allocate_info
	);

	m_device->immediate_submit([&](vk::CommandBuffer&& cmd) {
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
		  static_cast<uint32_t>(bufferCopies.size()),
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
			{},                       // flags
			texture.image,            // image
			vk::ImageViewType::eCube, // viewType
			texture.format,           // format

			/* components */
			vk::ComponentMapping {
			  // Don't swizzle the colors around...
			  vk::ComponentSwizzle::eIdentity, // r
			  vk::ComponentSwizzle::eIdentity, // g
			  vk::ComponentSwizzle::eIdentity, // b
			  vk::ComponentSwizzle::eIdentity, // a
			},

			/* subresourceRange */
			vk::ImageSubresourceRange {
			  vk::ImageAspectFlagBits::eColor, // aspectMask
			  0u,                              // baseMipLevel
			  texture.mip_levels,              // levelCount
			  0u,                              // baseArrayLayer
			  6u,                              // layerCount
			},
		};

		texture.image_view = m_device->logical.createImageView(
		  image_view_info,
		  nullptr
		);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// Create image samplers
	{
		vk::SamplerCreateInfo samplerCreateInfo {
			{},
			vk::Filter::eLinear,                  // magFilter
			vk::Filter::eLinear,                  // minFilter
			vk::SamplerMipmapMode::eLinear,       // mipmapMode
			vk::SamplerAddressMode::eClampToEdge, // addressModeU
			vk::SamplerAddressMode::eClampToEdge, // addressModeV
			vk::SamplerAddressMode::eClampToEdge, // addressModeW
			0.0f,                                 // mipLodBias

			// #TODO ANISOTROPY
			VK_FALSE, // anisotropyEnable
			{},       // maxAnisotropy

			VK_FALSE,                               // compareEnable
			vk::CompareOp::eNever,                  // compareOp
			0.0f,                                   // minLod
			static_cast<float>(texture.mip_levels), // maxLod
			vk::BorderColor::eFloatOpaqueWhite,     // borderColor
			VK_FALSE,                               // unnormalizedCoordinates
		};

		/// @todo Should we separate samplers and textures?
		texture.sampler = m_device->logical.createSampler(
		  samplerCreateInfo,
		  nullptr
		);
	}

	texture.descriptor_info = {
		texture.sampler,    // sampler
		texture.image_view, // imageView
		texture.layout,     // imageLayout
	};

	/////////////////////////////////////////////////////////////////////////////////
	/// Cleanup
	m_device->allocator.destroyBuffer(staging_buffer, staging_buffer);
	ktxTexture_Destroy(texture_ktx);

	return &m_textures[HashStr(name.c_str())];
}

void TextureSystem::blit_iamge(
  vk::CommandBuffer cmd,
  AllocatedImage image,
  uint32_t mip_index,
  int32_t& mip_width,
  int32_t& mip_height
)
{
	vk::ImageBlit blit {
		/* srcSubresource */
		vk::ImageSubresourceLayers {
		  vk::ImageAspectFlagBits::eColor, // aspectMask
		  mip_index - 1u,                  // mipLevel
		  0u,                              // baseArrayLayer
		  1u,                              // layerCount
		},
		/* srcOffsets */
		{
		  vk::Offset3D { 0, 0, 0 },
		  vk::Offset3D { mip_width, mip_height, 1 },
		},
		/* dstSubresource */
		vk::ImageSubresourceLayers {
		  vk::ImageAspectFlagBits::eColor, // aspectMask
		  mip_index,                       // mipLevel
		  0u,                              // baseArrayLayer
		  1u,                              // layerCount
		},
		/* dstOffsets */
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
	for (auto& [key, value] : m_textures)
	{
		m_device->logical.destroySampler(value.sampler, nullptr);
		m_device->logical.destroyImageView(value.image_view, nullptr);
		m_device->allocator.destroyImage(value.image, value.image);
	}
}

void TextureSystem::transition_layout(
  Texture& texture,
  vk::CommandBuffer cmdBuffer,
  uint32_t baseMipLevel,
  uint32_t levelCount,
  uint32_t layerCount,
  vk::ImageLayout oldLayout,
  vk::ImageLayout newLayout
)
{
	// Memory barrier
	vk::ImageMemoryBarrier imageMemBarrier {
		{},                      // srcAccessMask
		{},                      // dstAccessMask
		oldLayout,               // oldLayout
		newLayout,               // newLayout
		VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
		VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
		texture.image,           // image

		/* subresourceRange */
		vk::ImageSubresourceRange {
		  vk::ImageAspectFlagBits::eColor, // aspectMask
		  baseMipLevel,                    // baseMipLevel
		  levelCount,                      // levelCount
		  0u,                              // baseArrayLayer
		  layerCount,                      // layerCount
		},
	};
	/* Specify the source & destination stages and access masks... */
	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;

	// Undefined -> TRANSFER DST
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
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
		/// @todo: Stringifier
		BVK_LOG(
		  LogLvl::eError,
		  "Texture transition layout to/from unexpected layout(s) \n {} -> {}",
		  (int32_t)oldLayout,
		  (int32_t)newLayout
		);
	}

	// Execute pipeline barrier
	cmdBuffer.pipelineBarrier(
	  srcStage,
	  dstStage,
	  {},
	  0,
	  nullptr,
	  0,
	  nullptr,
	  1,
	  &imageMemBarrier
	);
}

} // namespace BINDLESSVK_NAMESPACE

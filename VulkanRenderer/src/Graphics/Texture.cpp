#include "Graphics/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(TextureCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_Allocator(createInfo.allocator)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Load the image
	uint8_t* imageData = stbi_load(createInfo.imagePath.c_str(), &m_Width, &m_Height, &m_Channels, 4u);
	ASSERT(imageData, "Failed to load image at: {}", createInfo.imagePath);
	m_ImageSize = m_Width * m_Height * 4u;

	m_MipLevels = std::floor(std::log2(std::max(m_Width, m_Height))) + 1;

	/////////////////////////////////////////////////////////////////////////////////
	// Create the image and staging buffer
	{
		// Image
		vk::ImageCreateInfo imageCreateInfo {
			{},                        // flags
			vk::ImageType::e2D,        // imageType
			vk::Format::eR8G8B8A8Srgb, // format

			/* extent */
			vk::Extent3D {
			    static_cast<uint32_t>(m_Width),  // width
			    static_cast<uint32_t>(m_Height), // height
			    1u,                              // depth
			},
			m_MipLevels,                                                                                                    // mipLevels
			1u,                                                                                                             // arrayLayers
			vk::SampleCountFlagBits::e1,                                                                                    // samples
			vk::ImageTiling::eOptimal,                                                                                      // tiling
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, // usage
			vk::SharingMode::eExclusive,                                                                                    // sharingMode
			0u,                                                                                                             // queueFamilyIndexCount
			nullptr,                                                                                                        // queueFamilyIndices
			vk::ImageLayout::eUndefined,                                                                                    // initialLayout
		};

		vma::AllocationCreateInfo imageAllocInfo({},
		                                         vma::MemoryUsage::eGpuOnly,
		                                         vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_Image = m_Allocator.createImage(imageCreateInfo, imageAllocInfo);

		// Staging buffer
		vk::BufferCreateInfo stagingBufferCrateInfo {
			{},                                    // flags
			m_ImageSize,                           // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		vma::AllocationCreateInfo bufferAllocInfo({}, vma::MemoryUsage::eCpuOnly, {});
		m_StagingBuffer = m_Allocator.createBuffer(stagingBufferCrateInfo, bufferAllocInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write the image data
	{
		// Copy data to staging buffer
		void* stagingMap = m_Allocator.mapMemory(m_StagingBuffer);
		memcpy(stagingMap, imageData, m_ImageSize);
		m_Allocator.unmapMemory(m_StagingBuffer);

		// Allocate cmd buffer
		vk::CommandBufferAllocateInfo allocInfo {
			createInfo.commandPool,           // commandPool
			vk::CommandBufferLevel::ePrimary, // level
			1u,                               // commandBufferCount
		};
		vk::CommandBuffer cmdBuffer;
		cmdBuffer = m_LogicalDevice.allocateCommandBuffers(allocInfo)[0];

		// Begin cmd buffer
		vk::CommandBufferBeginInfo beginInfo {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
		};

		cmdBuffer.begin(beginInfo);

		// Move data from staging buffer to image
		TransitionLayout(cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(cmdBuffer);

		/////////////////////////////////////////////////////////////////////////////////
		// Generate mipmaps
		{
			// Check if image format supports linear blitting
			vk::FormatProperties formatProperties = createInfo.physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Srgb);

			if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
			{
				throw std::runtime_error("Texture image format does not support linear blitting");
			}

			vk::ImageMemoryBarrier barrier {
				{},                          // srcAccessmask
				{},                          // dstAccessMask
				vk::ImageLayout::eUndefined, // oldLayout
				vk::ImageLayout::eUndefined, // newLayout
				VK_QUEUE_FAMILY_IGNORED,     // srcQueueFamilyIndex
				VK_QUEUE_FAMILY_IGNORED,     // dstQueueFamilyIndex
				m_Image,                     // image

				vk::ImageSubresourceRange {
				    vk::ImageAspectFlagBits::eColor, // aspectMask
				    0u,                              // baseMipLevel
				    1u,                              // levelCount
				    0u,                              // baseArrayLayer
				    1u,                              // layerCount
				},
			};

			int32_t mipWidth  = m_Width;
			int32_t mipHeight = m_Height;

			for (uint32_t i = 1; i < m_MipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
				barrier.newLayout                     = vk::ImageLayout::eTransferSrcOptimal;
				barrier.dstAccessMask                 = vk::AccessFlagBits::eTransferRead;
				barrier.srcAccessMask                 = vk::AccessFlagBits::eTransferWrite;

				cmdBuffer.pipelineBarrier(
				    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
				    {},
				    0u, nullptr,
				    0u, nullptr,
				    1u, &barrier);


				vk::ImageBlit blit {
					/* srcSubresource */
					vk::ImageSubresourceLayers {
					    vk::ImageAspectFlagBits::eColor, // aspectMask
					    i - 1u,                          // mipLevel
					    0u,                              // baseArrayLayer
					    1u,                              // layerCount
					},

					/* srcOffsets */
					{
					    vk::Offset3D { 0, 0, 0 },
					    vk::Offset3D { mipWidth, mipHeight, 1 },
					},

					/* dstSubresource */
					vk::ImageSubresourceLayers {
					    vk::ImageAspectFlagBits::eColor, // aspectMask
					    i,                               // mipLevel
					    0u,                              // baseArrayLayer
					    1u,                              // layerCount
					},

					/* dstOffsets */
					{
					    vk::Offset3D { 0, 0, 0 },
					    vk::Offset3D { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 },
					},
				};

				cmdBuffer.blitImage(m_Image, vk::ImageLayout::eTransferSrcOptimal,
				                    m_Image, vk::ImageLayout::eTransferDstOptimal,
				                    1u, &blit, vk::Filter::eLinear);

				barrier.oldLayout     = vk::ImageLayout::eTransferSrcOptimal;
				barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

				cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
				                          {},
				                          0u, nullptr,
				                          0u, nullptr,
				                          1u, &barrier);

				if (mipWidth > 1)
					mipWidth /= 2;

				if (mipHeight > 1)
					mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
			barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout                     = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask                 = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask                 = vk::AccessFlagBits::eShaderRead;
			cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
			                          {},
			                          0u, nullptr,
			                          0u, nullptr,
			                          1u, &barrier);
		}

		// End & Submit cmd buffer
		cmdBuffer.end();
		vk::SubmitInfo submitInfo {
			0u,      // waitSemaphoreCount
			nullptr, // pWaitSemaphores
			nullptr, // pWaitDstStageMask

			1u,         // commandBufferCount
			&cmdBuffer, // pCommandBuffers

			0u,      // signalSemaphoreCount
			nullptr, // pSignalSemaphores
		};
		VKC(createInfo.graphicsQueue.submit(1u, &submitInfo, VK_NULL_HANDLE));

		// Wait for and free cmd buffer
		createInfo.graphicsQueue.waitIdle();
		m_LogicalDevice.freeCommandBuffers(createInfo.commandPool, 1u, &cmdBuffer);

		// Free image data
		stbi_image_free(imageData);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create image views
	{
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                        // flags
			m_Image,                   // image
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
			    m_MipLevels,                     // levelCount
			    0u,                              // baseArrayLayer
			    1u,                              // layerCount
			},
		};

		m_ImageView = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// Create image samplers
	{
		vk::SamplerCreateInfo samplerCreateInfo {
			{},
			vk::Filter::eLinear,              // magFilter
			vk::Filter::eLinear,              //minFilter
			vk::SamplerMipmapMode::eLinear,   // mipmapMode
			vk::SamplerAddressMode::eRepeat,  // addressModeU
			vk::SamplerAddressMode::eRepeat,  // addressModeV
			vk::SamplerAddressMode::eRepeat,  // addressModeW
			0.0f,                             // mipLodBias
			createInfo.anisotropyEnabled,     // anisotropyEnable
			createInfo.maxAnisotropy,         // maxAnisotropy
			VK_FALSE,                         // compareEnable
			vk::CompareOp::eAlways,           // compareOp
			0.0f,                             // minLod
			static_cast<float>(m_MipLevels),  // maxLod
			vk::BorderColor::eIntOpaqueBlack, // borderColor
			VK_FALSE,                         // unnormalizedCoordinates
		};

		m_Sampler = m_LogicalDevice.createSampler(samplerCreateInfo, nullptr);
	}
}

Texture::~Texture()
{
	m_LogicalDevice.destroySampler(m_Sampler, nullptr);
	m_LogicalDevice.destroyImageView(m_ImageView, nullptr);
	m_Allocator.destroyBuffer(m_StagingBuffer, m_StagingBuffer);
	m_Allocator.destroyImage(m_Image, m_Image);
}

void Texture::TransitionLayout(vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	// Memory barrier
	vk::ImageMemoryBarrier imageMemBarrier {
		{},                      // srcAccessMask
		{},                      // dstAccessMask
		oldLayout,               // oldLayout
		newLayout,               // newLayout
		VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
		VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
		m_Image,                 // image

		/* subresourceRange */
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eColor, // aspectMask
		    0u,                              // baseMipLevel
		    m_MipLevels,                     // levelCount
		    0u,                              // baseArrayLayer
		    1u,                              // layerCount
		},
	};
	/* Specify the source & destination stages and access masks... */
	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;

	// Undefined -> Transfer
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		imageMemBarrier.srcAccessMask = {};
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}

	// Transfer -> Shader read only
	if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		imageMemBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}

	// Execute pipeline barrier
	cmdBuffer.pipelineBarrier(
	    srcStage, dstStage,
	    {},
	    0, nullptr,
	    0, nullptr,
	    1, &imageMemBarrier);
}

void Texture::CopyBufferToImage(vk::CommandBuffer cmdBuffer)
{
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
		vk::Offset3D { 0, 0, 0 },                                                             // imageOffset
		vk::Extent3D { static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1u }, // imageExtent
	};

	cmdBuffer.copyBufferToImage(m_StagingBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, 1u, &bufferImageCopy);
}

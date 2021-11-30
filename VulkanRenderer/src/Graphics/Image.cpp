#include "Graphics/Image.h"

#include "Graphics/Device.h"
#include "Graphics/RendererCommand.h"

#include <stb_image.h>

Image::Image(Device* device, const std::string& path)
    : m_Device(device), m_Path(path)
{
	CreateImage(VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	TransitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	CopyBufferToImage(VK_IMAGE_ASPECT_COLOR_BIT);
	// TransitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	GenerateMipmaps();

	CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	CreateImageSampler();
}

Image::Image(Device* device, uint32_t width, uint32_t height)
    : m_Device(device)
    , m_Path("")
    , m_Width(width)
    , m_Height(height)
{
	VkFormat depthFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	                                           VK_IMAGE_TILING_OPTIMAL,
	                                           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	CreateImage(depthFormat, m_Device->sampleCount(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CreateImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	TransitionImageLayout(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
}

Image::Image(Device* device, uint32_t width, uint32_t height, VkSampleCountFlags sampleCount, VkFormat format)
    : m_Device(device)
    , m_Path("")
    , m_Width(width)
    , m_Height(height)
{
	CreateImage(format, m_Device->sampleCount(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CreateImageView(format, VK_IMAGE_ASPECT_COLOR_BIT);
}

Image::~Image()
{
	vkDestroySampler(m_Device->logical(), m_ImageSampler, nullptr);
	vkDestroyImageView(m_Device->logical(), m_ImageView, nullptr);
	vkDestroyImage(m_Device->logical(), m_Image, nullptr);
	vkFreeMemory(m_Device->logical(), m_ImageMemory, nullptr);
}

void Image::CreateImage(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties)
{
	m_ImageFormat = format;

	if (!m_Path.empty())
	{
		uint8_t* pixels = (uint8_t*)stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Components, 4);
		ASSERT(pixels, "failed to load image at: {}", m_Path);

		VkDeviceSize imageSize = m_Width * m_Height * 4;

		m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1u;

		m_StagingBuffer = std::make_unique<Buffer>(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* map = m_StagingBuffer->Map(imageSize);
		memcpy(map, pixels, imageSize);
		m_StagingBuffer->Unmap();

		stbi_image_free(pixels);
	}

	// image create-info
	VkImageCreateInfo imageCreateInfo {
		.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags     = 0x0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format    = format,
		.extent    = {
            .width  = static_cast<uint32_t>(m_Width),
            .height = static_cast<uint32_t>(m_Height),
            .depth  = 1u,
        },
		.mipLevels     = m_MipLevels,
		.arrayLayers   = 1u,
		.samples       = sampleCount,
		.tiling        = tiling,
		.usage         = usageFlags,
		.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VKC(vkCreateImage(m_Device->logical(), &imageCreateInfo, nullptr, &m_Image));

	// memory requirements
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_Device->logical(), m_Image, &memoryRequirements);

	// memory allocate-info
	VkMemoryAllocateInfo allocInfo {
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize  = memoryRequirements.size,
		.memoryTypeIndex = FetchMemoryType(memoryRequirements.memoryTypeBits, memoryProperties),
	};

	VKC(vkAllocateMemory(m_Device->logical(), &allocInfo, nullptr, &m_ImageMemory));
	vkBindImageMemory(m_Device->logical(), m_Image, m_ImageMemory, 0ull);
}

void Image::CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags)
{
	// image view create-info
	VkImageViewCreateInfo viewCreateInfo {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = m_Image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format   = format,

		.subresourceRange = {
		    .aspectMask     = aspectFlags,
		    .baseMipLevel   = 0,
		    .levelCount     = m_MipLevels,
		    .baseArrayLayer = 0,
		    .layerCount     = 1,
		},
	};

	VKC(vkCreateImageView(m_Device->logical(), &viewCreateInfo, nullptr, &m_ImageView));
}

void Image::CreateImageSampler()
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_Device->physical(), &deviceProperties);

	// sampler create-info
	VkSamplerCreateInfo samplerCreateInfo {
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter               = VK_FILTER_LINEAR,
		.minFilter               = VK_FILTER_LINEAR,
		.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias              = 0.0f,
		.anisotropyEnable        = VK_FALSE,
		.maxAnisotropy           = 1.0f,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_ALWAYS,
		.minLod                  = 0.0f,
		.maxLod                  = static_cast<float>(m_MipLevels),
		.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	vkCreateSampler(m_Device->logical(), &samplerCreateInfo, nullptr, &m_ImageSampler);
}

void Image::TransitionImageLayout(VkFormat format, VkImageLayout newLayout, VkImageAspectFlags aspectFlags)
{
	VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

	VkImageMemoryBarrier barrier {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask       = 0x0,
		.dstAccessMask       = 0x0,
		.oldLayout           = m_OldLayout,
		.newLayout           = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = m_Image,
		.subresourceRange    = {
            .aspectMask     = aspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = m_MipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1u,
        },
	};

	VkPipelineStageFlags sourceStage      = 0x0;
	VkPipelineStageFlags destinationStage = 0x0;

	if (m_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0x0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (m_OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (m_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		ASSERT(false, "Bruh moment");
	}

	vkCmdPipelineBarrier(commandBuffer,
	                     sourceStage, destinationStage, /* TODO */
	                     0x0,
	                     0u, nullptr,
	                     0u, nullptr,
	                     1u, &barrier);

	RendererCommand::EndOneTimeCommand(&commandBuffer);

	m_OldLayout = newLayout;
}

void Image::CopyBufferToImage(VkImageAspectFlags aspectFlags)
{
	VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

	VkBufferImageCopy region {
		.bufferOffset      = 0u,
		.bufferRowLength   = 0u,
		.bufferImageHeight = 0u,
		.imageSubresource  = {
            .aspectMask     = aspectFlags,
            .mipLevel       = 0u,
            .baseArrayLayer = 0u,
            .layerCount     = 1u,
        },
		.imageOffset = {
		    .x = 0,
		    .y = 0,
		    .z = 0,
		},
		.imageExtent = {
		    .width  = static_cast<uint32_t>(m_Width),
		    .height = static_cast<uint32_t>(m_Height),
		    .depth  = 1u,
		},
	};

	vkCmdCopyBufferToImage(commandBuffer, *m_StagingBuffer->GetBuffer(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);
	RendererCommand::EndOneTimeCommand(&commandBuffer);
}

uint32_t Image::FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_Device->physical(), &physicalMemoryProperties);

	for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (physicalMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to fetch required memory type!");
	return -1;
}


VkFormat Image::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlagBits features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_Device->physical(), format, &properties);
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
			return format;
	}

	ASSERT(false, "Failed to find supported format");
}

void Image::GenerateMipmaps()
{
	VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

	VkImageMemoryBarrier barrier {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = m_Image,
		.subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount     = 1u,
            .baseArrayLayer = 0u,
            .layerCount     = 1u,
        },
	};

	int mipWidth  = m_Width;
	int mipHeight = m_Height;

	for (uint32_t i = 1u; i < m_MipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1u;
		barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
		                     0u, nullptr,
		                     0u, nullptr,
		                     1, &barrier);

		VkImageBlit blit {
			.srcSubresource = {
			    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			    .mipLevel       = i - 1u,
			    .baseArrayLayer = 0u,
			    .layerCount     = 1u,
			},

			.srcOffsets = {
			    { 0, 0, 0 },
			    { mipWidth, mipHeight, 1u },
			},

			.dstSubresource = {
			    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			    .mipLevel       = i,
			    .baseArrayLayer = 0u,
			    .layerCount     = 1u,
			},

			.dstOffsets = {
			    { 0, 0, 0 },
			    { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 },
			},
		};

		vkCmdBlitImage(commandBuffer,
		               m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1u, &blit,
		               VK_FILTER_NEAREST);

		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
		                     0u, nullptr,
		                     0u, nullptr,
		                     1u, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;

		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
	barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &barrier);

	RendererCommand::EndOneTimeCommand(&commandBuffer);
}
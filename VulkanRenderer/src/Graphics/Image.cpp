#include "Graphics/Image.h"

#include "Graphics/Device.h"
#include "Graphics/RendererCommand.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(Device* device, const std::string& path)
    : m_Device(device), m_Path(path)
{
    CreateImage();
    TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage();
    TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    CreateImageView();
    CreateImageSampler();
}

Image::~Image()
{
    vkDestroySampler(m_Device->logical(), m_ImageSampler, nullptr);
    vkDestroyImageView(m_Device->logical(), m_ImageView, nullptr);
    vkDestroyImage(m_Device->logical(), m_Image, nullptr);
    vkFreeMemory(m_Device->logical(), m_ImageMemory, nullptr);
}

void Image::CreateImage()
{
    uint8_t* pixels = (uint8_t*)stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Components, 4);
    ASSERT(pixels, "failed to load image at: {}", m_Path);

    VkDeviceSize imageSize = m_Width * m_Height * 4;

    m_StagingBuffer = std::make_unique<Buffer>(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* map = m_StagingBuffer->Map(imageSize);
    memcpy(map, pixels, imageSize);
    m_StagingBuffer->Unmap();

    stbi_image_free(pixels);

    // image create-info
    VkImageCreateInfo imageCreateInfo {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags     = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format    = VK_FORMAT_R8G8B8A8_SRGB,
        .extent    = {
            .width  = static_cast<uint32_t>(m_Width),
            .height = static_cast<uint32_t>(m_Height),
            .depth  = 1u,
        },
        .mipLevels     = 1u,
        .arrayLayers   = 1u,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
        .memoryTypeIndex = FetchMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VKC(vkAllocateMemory(m_Device->logical(), &allocInfo, nullptr, &m_ImageMemory));
    vkBindImageMemory(m_Device->logical(), m_Image, m_ImageMemory, 0ull);
}

void Image::CreateImageView()
{
    // image view create-info
    VkImageViewCreateInfo viewCreateInfo {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = m_Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = VK_FORMAT_R8G8B8A8_SRGB,

        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
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
        .maxLod                  = 0.0f,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    vkCreateSampler(m_Device->logical(), &samplerCreateInfo, nullptr, &m_ImageSampler);
}

void Image::TransitionImageLayout(VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

    VkImageMemoryBarrier barrier {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = NULL,
        .dstAccessMask       = NULL,
        .oldLayout           = m_OldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = m_Image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1u,
        },

    };

    VkPipelineStageFlags sourceStage      = NULL;
    VkPipelineStageFlags destinationStage = NULL;

    if (m_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = NULL;
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
    else
    {
        ASSERT(false, "Bruh moment");
    }

    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage, destinationStage, /* TODO */
                         NULL,
                         0u, nullptr,
                         0u, nullptr,
                         1u, &barrier);

    RendererCommand::EndOneTimeCommand(&commandBuffer);

    m_OldLayout = newLayout;
}

void Image::CopyBufferToImage()
{
    VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

    VkBufferImageCopy region {
        .bufferOffset      = 0u,
        .bufferRowLength   = 0u,
        .bufferImageHeight = 0u,
        .imageSubresource  = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0u,
            .baseArrayLayer = 0u,
            .layerCount     = 1u,
        },
        .imageOffset = 0u,
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

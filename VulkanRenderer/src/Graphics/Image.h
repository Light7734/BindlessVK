#pragma once

#include "Core/Base.h"
#include "Graphics/Buffers.h"

class Image
{
private:
	class Device* m_Device = nullptr;

	const std::string m_Path;

	int m_Width      = 0;
	int m_Height     = 0;
	int m_Components = 0;

	uint32_t m_MipLevels = 1u;

	VkImage m_Image              = VK_NULL_HANDLE;
	VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
	VkImageView m_ImageView      = VK_NULL_HANDLE;
	VkSampler m_ImageSampler     = VK_NULL_HANDLE;

	VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;

	VkImageLayout m_OldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	std::unique_ptr<Buffer> m_StagingBuffer = nullptr;


public:
	// Color Image
	Image(Device* device, const std::string& path);

	// Depth Image
	Image(Device* device, uint32_t width, uint32_t height);

	// MSAA Image
	Image(Device* device, uint32_t width, uint32_t height, VkSampleCountFlags sampleCount, VkFormat format);

	~Image();

	inline VkImageView GetImageView() const { return m_ImageView; }
	inline VkSampler GetSampler() const { return m_ImageSampler; }

	inline VkFormat GetFormat() const { return m_ImageFormat; }

private:
	void CreateImage(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties);
	void CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateImageSampler();

	void TransitionImageLayout(VkFormat format, VkImageLayout newLayout, VkImageAspectFlags aspectFlags);
	void CopyBufferToImage(VkImageAspectFlags aspectFlags);
	uint32_t FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

private:
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlagBits features) const;
	inline bool HasStencilComponent(VkFormat format) const { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }

	void GenerateMipmaps();
};
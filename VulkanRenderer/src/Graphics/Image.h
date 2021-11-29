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

	VkImage m_Image;
	VkDeviceMemory m_ImageMemory;
	VkImageView m_ImageView;
	VkSampler m_ImageSampler;

	VkImageLayout m_OldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	std::unique_ptr<Buffer> m_StagingBuffer;

public:
	Image(Device* device, const std::string& path);
	~Image();

	inline VkImageView GetImageView() const { return m_ImageView; }
	inline VkSampler GetSampler() const { return m_ImageSampler; }

private:
	void CreateImage();
	void CreateImageView();
	void CreateImageSampler();

	void TransitionImageLayout(VkImageLayout newLayout);
	void CopyBufferToImage();
	uint32_t FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);
};
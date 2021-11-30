#pragma once

#include "Core/Base.h"
#include "Graphics/Device.h"
#include "Graphics/Image.h"

#include <volk.h>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	inline bool IsSuitableForRendering() const { !formats.empty() && !presentModes.empty(); }

	operator bool() const { return IsSuitableForRendering(); }
};

class Swapchain
{
public:
	Swapchain(class Window* window, class Device* device, Swapchain* old = nullptr);
	~Swapchain();

	// Not copyable/movable
	Swapchain(Swapchain&&)      = delete;
	Swapchain(const Swapchain&) = delete;
	Swapchain operator=(Swapchain&&) = delete;
	Swapchain operator=(const Swapchain&) = delete;

	// Getters
	inline VkSwapchainKHR* GetVkSwapchain() { return &m_Swapchain; }

	inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
	inline VkFramebuffer GetFramebuffer(uint32_t index) const { return m_Framebuffers[index]; }

	inline VkExtent2D GetExtent() const { return m_Extent; }

	uint32_t FetchNextImage(VkSemaphore semaphore);

	inline size_t GetImageCount() const { return m_Images.size(); }

private:
	class Window* m_Window;
	class Device* m_Device;

	// swap chain
	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;

	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_Extent;

	std::unique_ptr<Image> m_DepthImage;
	std::unique_ptr<Image> m_MultisampleImage;

	VkRenderPass m_RenderPass;

	SwapchainSupportDetails m_SwapChainDetails;

private:
	void CreateSwapchain(Swapchain* old);
	void CreateImageViews();
	void CreateRenderPass();
	void CreateFramebuffers();

	void FetchSwapchainSupportDetails();
};
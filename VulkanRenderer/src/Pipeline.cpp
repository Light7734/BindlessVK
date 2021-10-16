#include "Pipeline.h"

#include <glfw/glfw3.h>

Pipeline::Pipeline() :
	m_Swapchain(VK_NULL_HANDLE)
{
}

Pipeline::~Pipeline()
{
	for (auto swapchainImageView : m_SwapchainImageViews)
		vkDestroyImageView(m_SharedContext.logicalDevice, swapchainImageView, nullptr);

	vkDestroySwapchainKHR(m_SharedContext.logicalDevice, m_Swapchain, nullptr);
}

void Pipeline::CreateSwapchain()
{
	// pick a decent swap chain surface format
	VkSurfaceFormatKHR swapChainSurfaceFormat{};
	for (const auto& surfaceFormat : m_SwapChainDetails.formats)
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			swapChainSurfaceFormat = surfaceFormat;

	if (swapChainSurfaceFormat.format == VK_FORMAT_UNDEFINED)
		m_SwapchainImageFormat = swapChainSurfaceFormat.format;

	// pick a decent swap chain present mode
	VkPresentModeKHR swapChainPresentMode{};
	for (const auto& presentMode : m_SwapChainDetails.presentModes)
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			swapChainPresentMode = presentMode;

	if (m_SwapChainDetails.capabilities.currentExtent.width != UINT32_MAX)
		m_SwapchainExtent = m_SwapChainDetails.capabilities.currentExtent;
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_SharedContext.windowHandle, &width, &height);

		m_SwapchainExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		m_SwapchainExtent.width = std::clamp(m_SwapchainExtent.width, m_SwapChainDetails.capabilities.minImageExtent.width,
			m_SwapChainDetails.capabilities.maxImageExtent.width);

		m_SwapchainExtent.height = std::clamp(m_SwapchainExtent.height, m_SwapChainDetails.capabilities.minImageExtent.height,
			m_SwapChainDetails.capabilities.maxImageExtent.height);
	}

	uint32_t imageCount = m_SwapChainDetails.capabilities.minImageCount + 1;
	if (m_SwapChainDetails.capabilities.maxImageCount > 0 && imageCount > m_SwapChainDetails.capabilities.maxImageCount)
		imageCount = m_SwapChainDetails.capabilities.maxImageCount;

	// swapchain create-info
	VkSwapchainCreateInfoKHR swapchainCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,

		.surface = m_SharedContext.surface,

		.minImageCount = imageCount,
		.imageFormat = swapChainSurfaceFormat.format,
		.imageColorSpace = swapChainSurfaceFormat.colorSpace,
		.imageExtent = m_SwapchainExtent,
		.imageArrayLayers = 1u,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = m_SharedContext.queueFamilies.graphics.value() == m_SharedContext.queueFamilies.present.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,

		.queueFamilyIndexCount = m_SharedContext.queueFamilies.graphics.value() == m_SharedContext.queueFamilies.present.value() ? 0u : 2u,
		.pQueueFamilyIndices = m_SharedContext.queueFamilies.graphics.value() == m_SharedContext.queueFamilies.present.value() ? nullptr : m_SharedContext.queueFamilies.indices.data(),
		.preTransform = m_SwapChainDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapChainPresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	VKC(vkCreateSwapchainKHR(m_SharedContext.logicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain));

	vkGetSwapchainImagesKHR(m_SharedContext.logicalDevice, m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_SharedContext.logicalDevice, m_Swapchain, &imageCount, m_SwapchainImages.data());
}

void Pipeline::CreateImageViews()
{
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (int i = 0; i < m_SwapchainImages.size(); i++)
	{
		// image view create-info
		VkImageViewCreateInfo imageViewCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_SwapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_SwapchainImageFormat,
			.components
			{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0u,
				.levelCount = 0u,
				.baseArrayLayer = 0u,
				.layerCount = 0u,
			}
		};

		VKC(vkCreateImageView(m_SharedContext.logicalDevice, &imageViewCreateInfo, nullptr, &m_SwapchainImageViews[i]));
	}
}

void Pipeline::FetchSwapchainSupportDetails()
{
	// fetch device surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_SharedContext.physicalDevice, m_SharedContext.surface, &m_SwapChainDetails.capabilities);

	// fetch device surface formats
	uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_SharedContext.physicalDevice, m_SharedContext.surface, &formatCount, nullptr);
	ASSERT(formatCount, "GraphicsContext::FetchSwapchainSupportDetails: no physical device surface format");

	m_SwapChainDetails.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_SharedContext.physicalDevice, m_SharedContext.surface, &formatCount, m_SwapChainDetails.formats.data());

	// fetch device surface format modes
	uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_SharedContext.physicalDevice, m_SharedContext.surface, &presentModeCount, nullptr);
	ASSERT(presentModeCount, "GraphicsContext::FetchSwapchainSupportDetails: no phyiscal device surface present mode");

	m_SwapChainDetails.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_SharedContext.physicalDevice, m_SharedContext.surface, &presentModeCount, m_SwapChainDetails.presentModes.data());
}
#pragma once

#include "Base.h"

#include "Shader.h"

#include <volk.h>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	inline bool IsSuitableForRendering() const { !formats.empty() && !presentModes.empty(); }

	operator bool() const { return IsSuitableForRendering(); }
};

class Pipeline
{
private:
	// context
	SharedContext m_SharedContext;

	// swap chain
	VkSwapchainKHR m_Swapchain;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;

	SwapchainSupportDetails m_SwapChainDetails;

public:
	Pipeline();
	~Pipeline();

private:
	void CreateSwapchain();
	void CreateImageViews();

	void FetchSwapchainSupportDetails();

};
#include "BindlessVk/Renderer.hpp"

#include "BindlessVk/Types.hpp"

namespace BINDLESSVK_NAMESPACE {

void Renderer::Reset()
{
	if (!m_Device)
	{
		return;
	}

	DestroySwapchainResources();

	for (uint32_t i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_Device->logical.destroyFence(m_RenderFences[i], nullptr);
		m_Device->logical.destroySemaphore(m_RenderSemaphores[i], nullptr);
		m_Device->logical.destroySemaphore(m_PresentSemaphores[i], nullptr);
	}

	m_RenderGraph.Reset();
	m_Device->logical.resetDescriptorPool(m_DescriptorPool);
	m_Device->logical.destroyDescriptorPool(m_DescriptorPool, nullptr);

	m_Device->logical.destroySwapchainKHR(m_Swapchain);
}

void Renderer::Init(const Renderer::CreateInfo& info)
{
	m_Device = info.device;

	CreateSyncObjects();
	CreateDescriptorPools();
	CreateCmdBuffers();

	// Recreates swapchain resources
	OnSwapchainInvalidated();
}

void Renderer::OnSwapchainInvalidated()
{
	DestroySwapchainResources();

	CreateSwapchain();

	m_RenderGraph.OnSwapchainInvalidated(m_SwapchainImages, m_SwapchainImageViews);

	m_SwapchainInvalidated = false;
	m_Device->logical.waitIdle();
}

void Renderer::DestroySwapchainResources()
{
	if (!m_Swapchain)
	{
		return;
	}

	m_Device->logical.waitIdle();

	for (const auto& imageView : m_SwapchainImageViews)
	{
		m_Device->logical.destroyImageView(imageView);
	}

	m_SwapchainImageViews.clear();
	m_SwapchainImages.clear();
};

void Renderer::CreateSyncObjects()
{
	for (uint32_t i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo {};

		vk::FenceCreateInfo fenceCreateInfo {
			vk::FenceCreateFlagBits::eSignaled, // flags
		};

		m_RenderFences[i]      = m_Device->logical.createFence(fenceCreateInfo, nullptr);
		m_RenderSemaphores[i]  = m_Device->logical.createSemaphore(semaphoreCreateInfo, nullptr);
		m_PresentSemaphores[i] = m_Device->logical.createSemaphore(semaphoreCreateInfo, nullptr);
	}
}

void Renderer::CreateDescriptorPools()
{
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{ vk::DescriptorType::eSampler, 8000 },
		{ vk::DescriptorType::eCombinedImageSampler, 8000 },
		{ vk::DescriptorType::eSampledImage, 8000 },
		{ vk::DescriptorType::eStorageImage, 8000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 8000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 8000 },
		{ vk::DescriptorType::eUniformBuffer, 8000 },
		{ vk::DescriptorType::eStorageBuffer, 8000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 8000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 8000 },
		{ vk::DescriptorType::eInputAttachment, 8000 }
	};
	// descriptorPoolSizes.push_back(VkDescriptorPoolSize {});

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {
		vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
		    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // flags

		100,                                     // maxSets
		static_cast<uint32_t>(poolSizes.size()), // poolSizeCount
		poolSizes.data(),                        // pPoolSizes
	};

	m_DescriptorPool = m_Device->logical.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
}

void Renderer::CreateSwapchain()
{
	const uint32_t minImage = m_Device->surfaceCapabilities.minImageCount;
	const uint32_t maxImage = m_Device->surfaceCapabilities.maxImageCount;

	uint32_t imageCount = maxImage >= DESIRED_SWAPCHAIN_IMAGES && minImage <= DESIRED_SWAPCHAIN_IMAGES ? DESIRED_SWAPCHAIN_IMAGES :
	                      maxImage == 0ul && minImage <= DESIRED_SWAPCHAIN_IMAGES                      ? DESIRED_SWAPCHAIN_IMAGES :
	                      minImage <= 2ul && maxImage >= 2ul                                           ? 2ul :
	                      minImage == 0ul && maxImage >= 2ul                                           ? 2ul :
	                                                                                                     minImage;
	// Create swapchain
	bool sameQueueIndex           = m_Device->graphicsQueueIndex == m_Device->presentQueueIndex;
	vk::SwapchainKHR oldSwapchain = m_Swapchain;

	vk::SwapchainCreateInfoKHR swapchainCreateInfo {
		{},                                                                          // flags
		m_Device->surface,                                                           // surface
		imageCount,                                                                  // minImageCount
		m_Device->surfaceFormat.format,                                              // imageFormat
		m_Device->surfaceFormat.colorSpace,                                          // imageColorSpace
		m_Device->framebufferExtent,                                                 // imageExtent
		1u,                                                                          // imageArrayLayers
		vk::ImageUsageFlagBits::eColorAttachment,                                    // imageUsage
		sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
		sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
		sameQueueIndex ? nullptr : &m_Device->graphicsQueueIndex,                    // pQueueFamilyIndices
		m_Device->surfaceCapabilities.currentTransform,                              // preTransform
		vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // compositeAlpha -> No alpha-blending between multiple windows
		m_Device->presentMode,                                                       // presentMode
		VK_TRUE,                                                                     // clipped -> Don't render the obsecured pixels
		oldSwapchain,                                                                // oldSwapchain
	};

	m_Swapchain = m_Device->logical.createSwapchainKHR(swapchainCreateInfo, nullptr);
	m_Device->logical.destroySwapchainKHR(oldSwapchain);

	m_SwapchainImages = m_Device->logical.getSwapchainImagesKHR(m_Swapchain);

	for (uint32_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                             // flags
			m_SwapchainImages[i],           // image
			vk::ImageViewType::e2D,         // viewType
			m_Device->surfaceFormat.format, // format

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
			    vk::ImageAspectFlagBits::eColor, // Image will be used as color
			                                     // target // aspectMask
			    0,                               // No mipmaipping // baseMipLevel
			    1,                               // No levels // levelCount
			    0,                               // No nothin... // baseArrayLayer
			    1,                               // layerCount
			},
		};

		m_SwapchainImageViews.push_back(m_Device->logical.createImageView(imageViewCreateInfo, nullptr));
	}


	for (uint32_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		std::string imageName("swap_chain Image #" + std::to_string(i));
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)m_SwapchainImages[i],
		    imageName.c_str(),
		});

		std::string viewName("swap_chain ImageView #" + std::to_string(i));
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (uint64_t)(VkImageView)m_SwapchainImageViews[i],
		    viewName.c_str(),
		});
	}
}

void Renderer::CreateCmdBuffers()
{
	m_CommandBuffers.resize(BVK_MAX_FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo {
			m_Device->GetCmdPool(i),          // commandPool
			vk::CommandBufferLevel::ePrimary, // level
			1u,                               // commandBufferCount
		};

		m_CommandBuffers[i] = m_Device->logical.allocateCommandBuffers(cmdBufferAllocInfo)[0];
	}
}

void Renderer::BuildRenderGraph(RenderGraph::BuildInfo buildInfo)
{
	m_RenderGraph.Init({
	    .device              = m_Device,
	    .descriptorPool      = m_DescriptorPool,
	    .swapchainImages     = m_SwapchainImages,
	    .swapchainImageViews = m_SwapchainImageViews,
	});

	m_RenderGraph.Build(buildInfo);
}

void Renderer::BeginFrame(void* userPointer)
{
	m_RenderGraph.BeginFrame({
	    .graph       = &m_RenderGraph,
	    .pass        = nullptr,
	    .userPointer = userPointer,

	    .device = m_Device,
	    .cmd    = {},

	    .imageIndex = {},
	    .frameIndex = m_CurrentFrame,
	});
}

void Renderer::EndFrame(void* userPointer)
{
	if (m_SwapchainInvalidated)
		return;

	BVK_ASSERT(m_Device->logical.waitForFences(1u, &m_RenderFences[m_CurrentFrame], VK_TRUE, UINT64_MAX));
	BVK_ASSERT(m_Device->logical.resetFences(1u, &m_RenderFences[m_CurrentFrame]));

	uint32_t imageIndex;
	vk::Result result = m_Device->logical.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_RenderSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
	{
		m_Device->logical.waitIdle();
		m_SwapchainInvalidated = true;
		return;
	}
	else
	{
		BVK_ASSERT(result != vk::Result::eSuccess, "VkAcquireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR");
	}

	static uint32_t counter = 0u;

	m_Device->logical.resetCommandPool(m_Device->GetCmdPool(m_CurrentFrame));
	auto cmd = m_CommandBuffers[m_CurrentFrame];

	m_RenderGraph.EndFrame({
	    .graph       = &m_RenderGraph,
	    .pass        = nullptr,
	    .userPointer = userPointer,

	    .device = m_Device,
	    .cmd    = cmd,

	    .imageIndex = imageIndex,
	    .frameIndex = m_CurrentFrame,
	});

	counter++;

	SubmitQueue(m_RenderSemaphores[m_CurrentFrame], m_PresentSemaphores[m_CurrentFrame], m_RenderFences[m_CurrentFrame], cmd);
	PresentFrame(m_PresentSemaphores[m_CurrentFrame], imageIndex);

	m_CurrentFrame = (m_CurrentFrame + 1u) % BVK_MAX_FRAMES_IN_FLIGHT;
}

void Renderer::SubmitQueue(vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, vk::Fence signalFence, vk::CommandBuffer cmd)
{
	cmd.end();

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo {
		1u,               // waitSemaphoreCount
		&waitSemaphore,   // pWaitSemaphores
		&waitStage,       // pWaitDstStageMask
		1u,               // commandBufferCount
		&cmd,             // pCommandBuffers
		1u,               // signalSemaphoreCount
		&signalSemaphore, // pSignalSemaphores
	};

	BVK_ASSERT(m_Device->graphicsQueue.submit(1u, &submitInfo, signalFence));
}

void Renderer::PresentFrame(vk::Semaphore waitSemaphore, uint32_t imageIndex)
{
	vk::PresentInfoKHR presentInfo {
		1u,             // waitSemaphoreCount
		&waitSemaphore, // pWaitSemaphores
		1u,             // swapchainCount
		&m_Swapchain,   // pSwapchains
		&imageIndex,    // pImageIndices
		nullptr         // pResults
	};

	try
	{
		vk::Result result = m_Device->presentQueue.presentKHR(presentInfo);
		if (result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
		{
			m_Device->logical.waitIdle();
			m_SwapchainInvalidated = true;
			return;
		}
	}
	catch (vk::OutOfDateKHRError err) // OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	{
		m_Device->logical.waitIdle();
		m_SwapchainInvalidated = true;
		return;
	}
}
} // namespace BINDLESSVK_NAMESPACE

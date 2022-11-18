#include "BindlessVk/Renderer.hpp"

#include "BindlessVk/Types.hpp"

namespace BINDLESSVK_NAMESPACE {

Renderer::~Renderer()
{
	if (!m_Device)
	{
		return;
	}

	DestroySwapchain();

	m_Device->logical.destroyDescriptorPool(m_DescriptorPool, nullptr);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_Device->logical.destroyFence(m_RenderFences[i], nullptr);
		m_Device->logical.destroySemaphore(m_RenderSemaphores[i], nullptr);
		m_Device->logical.destroySemaphore(m_PresentSemaphores[i], nullptr);
	}

	m_Device->logical.destroyFence(m_UploadContext.fence);
}

void Renderer::Init(const Renderer::CreateInfo& info)
{
	m_Device = info.device;

	CreateSyncObjects();
	CreateDescriptorPools();
	RecreateSwapchainResources();
}

void Renderer::RecreateSwapchainResources()
{
	DestroySwapchain();

	m_SampleCount = m_Device->maxDepthColorSamples;

	CreateSwapchain();
	CreateCommandPool();

	m_SwapchainInvalidated = false;
	m_Device->logical.waitIdle();
}

void Renderer::DestroySwapchain()
{
	if (!m_Swapchain)
	{
		return;
	}

	m_Device->logical.waitIdle();
	//

	m_Device->logical.resetCommandPool(m_CommandPool);
	m_Device->logical.resetCommandPool(m_UploadContext.cmdPool);
	m_Device->logical.destroyCommandPool(m_UploadContext.cmdPool);
	m_Device->logical.destroyCommandPool(m_CommandPool);

	m_Device->logical.resetDescriptorPool(m_DescriptorPool);

	for (const auto& imageView : m_SwapchainImageViews)
	{
		m_Device->logical.destroyImageView(imageView);
	}

	m_Device->logical.destroySwapchainKHR(m_Swapchain);
};

void Renderer::CreateSyncObjects()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo {};

		vk::FenceCreateInfo fenceCreateInfo {
			vk::FenceCreateFlagBits::eSignaled, // flags
		};

		m_RenderFences[i]      = m_Device->logical.createFence(fenceCreateInfo, nullptr);
		m_RenderSemaphores[i]  = m_Device->logical.createSemaphore(semaphoreCreateInfo, nullptr);
		m_PresentSemaphores[i] = m_Device->logical.createSemaphore(semaphoreCreateInfo, nullptr);
	}

	m_UploadContext.fence = m_Device->logical.createFence({}, nullptr);
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
		vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind, // flags
		100,                                                // maxSets
		static_cast<uint32_t>(poolSizes.size()),            // poolSizeCount
		poolSizes.data(),                                   // pPoolSizes
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
	bool sameQueueIndex = m_Device->graphicsQueueIndex == m_Device->presentQueueIndex;
	vk::SwapchainCreateInfoKHR swapchainCreateInfo {
		{},                                                                          // flags
		m_Device->surface,                                                           // surface
		imageCount,                                                                  // minImageCount
		m_Device->surfaceFormat.format,                                              // imageFormat
		m_Device->surfaceFormat.colorSpace,                                          // imageColorSpace
		m_Device->surfaceCapabilities.currentExtent,                                 // imageExtent
		1u,                                                                          // imageArrayLayers
		vk::ImageUsageFlagBits::eColorAttachment,                                    // imageUsage
		sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
		sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
		sameQueueIndex ? nullptr : &m_Device->graphicsQueueIndex,                    // pQueueFamilyIndices
		m_Device->surfaceCapabilities.currentTransform,                              // preTransform
		vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // compositeAlpha -> No alpha-blending between multiple windows
		m_Device->presentMode,                                                       // presentMode
		VK_TRUE,                                                                     // clipped -> Don't render the obsecured pixels
		VK_NULL_HANDLE,                                                              // oldSwapchain
	};

	m_Swapchain       = m_Device->logical.createSwapchainKHR(swapchainCreateInfo, nullptr);
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

void Renderer::CreateCommandPool()
{
	// renderer
	vk::CommandPoolCreateInfo commandPoolCreateInfo {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
		m_Device->graphicsQueueIndex,                       // queueFamilyIndex
	};

	m_CommandPool           = m_Device->logical.createCommandPool(commandPoolCreateInfo, nullptr);
	m_UploadContext.cmdPool = m_Device->logical.createCommandPool(commandPoolCreateInfo, nullptr);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo {
		m_CommandPool,                    // commandPool
		vk::CommandBufferLevel::ePrimary, // level
		MAX_FRAMES_IN_FLIGHT,             // commandBufferCount
	};
	m_CommandBuffers = m_Device->logical.allocateCommandBuffers(cmdBufferAllocInfo);

	vk::CommandBufferAllocateInfo uploadContextCmdBufferAllocInfo {
		m_UploadContext.cmdPool,
		vk::CommandBufferLevel::ePrimary,
		1u,
	};
	m_UploadContext.cmdBuffer = m_Device->logical.allocateCommandBuffers(uploadContextCmdBufferAllocInfo)[0];
}

void Renderer::BuildRenderGraph(RenderGraph::BuildInfo buildInfo)
{
	m_RenderGraph.Init({
	    .device              = m_Device,
	    .descriptorPool      = m_DescriptorPool,
	    .commandPool         = m_CommandPool,
	    .swapchainImages     = m_SwapchainImages,
	    .swapchainImageViews = m_SwapchainImageViews,
	});

	m_RenderGraph.Build(buildInfo);
}

void Renderer::BeginFrame(void* userPointer)
{
	auto cmd = m_CommandBuffers[m_CurrentFrame];

	m_RenderGraph.BeginFrame({
	    .graph       = &m_RenderGraph,
	    .pass        = nullptr,
	    .userPointer = userPointer,

	    .logicalDevice = m_Device->logical,
	    .cmd           = {},

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

	auto cmd = m_CommandBuffers[m_CurrentFrame];

	m_RenderGraph.EndFrame({
	    .graph       = &m_RenderGraph,
	    .pass        = nullptr,
	    .userPointer = userPointer,

	    .logicalDevice = m_Device->logical,
	    .cmd           = cmd,

	    .imageIndex = imageIndex,
	    .frameIndex = m_CurrentFrame,
	});

	counter++;

	SubmitQueue(m_RenderSemaphores[m_CurrentFrame], m_PresentSemaphores[m_CurrentFrame], m_RenderFences[m_CurrentFrame], cmd);
	PresentFrame(m_PresentSemaphores[m_CurrentFrame], imageIndex);

	m_CurrentFrame = (m_CurrentFrame + 1u) % MAX_FRAMES_IN_FLIGHT;
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

void Renderer::ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function)
{
	vk::CommandBuffer cmd = m_UploadContext.cmdBuffer;

	vk::CommandBufferBeginInfo beginInfo {
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};

	cmd.begin(beginInfo);

	function(cmd);

	cmd.end();

	vk::SubmitInfo submitInfo { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };

	m_Device->graphicsQueue.submit(submitInfo, m_UploadContext.fence);
	BVK_ASSERT(m_Device->logical.waitForFences(m_UploadContext.fence, true, UINT_MAX));
	m_Device->logical.resetFences(m_UploadContext.fence);

	m_Device->logical.resetCommandPool(m_CommandPool, {});
}
} // namespace BINDLESSVK_NAMESPACE

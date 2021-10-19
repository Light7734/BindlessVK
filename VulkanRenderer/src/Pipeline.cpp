#include "Pipeline.h"

#include <glfw/glfw3.h>

Pipeline::Pipeline(SharedContext sharedContext, std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages) :
	m_SharedContext(sharedContext),
	m_Pipeline(VK_NULL_HANDLE),
	m_PipelineLayout(VK_NULL_HANDLE),
	m_RenderPass(VK_NULL_HANDLE),
	m_Swapchain(VK_NULL_HANDLE),
	m_SwapchainImageFormat(VK_FORMAT_UNDEFINED)
{
	FetchSwapchainSupportDetails();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreatePipelineLayout();
	CreatePipeline(shaderStages);
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSemaphores();
}

Pipeline::~Pipeline()
{
	vkDestroySemaphore(m_SharedContext.logicalDevice, m_ImageAvailableSemaphor, nullptr);
	vkDestroySemaphore(m_SharedContext.logicalDevice, m_RenderFinishedSemaphor, nullptr);

	vkDestroyCommandPool(m_SharedContext.logicalDevice, m_CommandPool, nullptr);

	for (auto framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_SharedContext.logicalDevice, framebuffer, nullptr);

	vkDestroyPipeline(m_SharedContext.logicalDevice, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_SharedContext.logicalDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_SharedContext.logicalDevice, m_RenderPass, nullptr);

	for (auto swapchainImageView : m_SwapchainImageViews)
		vkDestroyImageView(m_SharedContext.logicalDevice, swapchainImageView, nullptr);

	vkDestroySwapchainKHR(m_SharedContext.logicalDevice, m_Swapchain, nullptr);
}

uint32_t Pipeline::AquireNextImage()
{
	uint32_t out;
	VKC(vkAcquireNextImageKHR(m_SharedContext.logicalDevice, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphor, VK_NULL_HANDLE, &out));

	return out;
}

void Pipeline::SubmitCommandBuffer(uint32_t imageIndex)
{
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// submit info
	VkSubmitInfo submitInfo
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_ImageAvailableSemaphor,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1u,
		.pCommandBuffers = &m_CommandBuffers[imageIndex],
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores = &m_RenderFinishedSemaphor,
	};

	// submit queue
	VKC(vkQueueSubmit(m_SharedContext.graphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR presentInfo
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_RenderFinishedSemaphor,
		.swapchainCount = 1u,
		.pSwapchains = &m_Swapchain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,
	};

	VKC(vkQueuePresentKHR(m_SharedContext.presentQueue, &presentInfo));
}

void Pipeline::CreateSwapchain()
{
	// pick a decent swap chain surface format
	VkSurfaceFormatKHR swapChainSurfaceFormat{};
	for (const auto& surfaceFormat : m_SwapChainDetails.formats)
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			swapChainSurfaceFormat = surfaceFormat;

	if (swapChainSurfaceFormat.format == VK_FORMAT_UNDEFINED)
		swapChainSurfaceFormat = m_SwapChainDetails.formats[0];

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
				.levelCount = 1u,
				.baseArrayLayer = 0u,
				.layerCount = 1u,
			}
		};

		// create image view
		VKC(vkCreateImageView(m_SharedContext.logicalDevice, &imageViewCreateInfo, nullptr, &m_SwapchainImageViews[i]));
	}
}

void Pipeline::CreateRenderPass()
{
	/// attachment description
	VkAttachmentDescription attachmentDescription
	{
		.format = m_SwapchainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	// attachment reference
	VkAttachmentReference attachmentReference
	{
		.attachment = 0u,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	// subpass description
	VkSubpassDescription subpassDescription
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachmentReference
	};

	// subpass dependency
	VkSubpassDependency subpassDependency
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0u,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0u,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	// render pass create-info
	VkRenderPassCreateInfo renderPassCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1u,
		.pAttachments = &attachmentDescription,
		.subpassCount = 1u,
		.pSubpasses = &subpassDescription,
		.dependencyCount = 1u,
		.pDependencies = &subpassDependency,
	};

	// create render pass
	VKC(vkCreateRenderPass(m_SharedContext.logicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass));

}

void Pipeline::CreatePipelineLayout()
{
	// pipeline layout create-info
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0u,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0u,
		.pPushConstantRanges = nullptr
	};

	// create pipeline layout
	VKC(vkCreatePipelineLayout(m_SharedContext.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
}

void Pipeline::CreatePipeline(std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages)
{
	// pipeline Vertex state create-info
	VkPipelineVertexInputStateCreateInfo pipelineVertexStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0u,
		.vertexAttributeDescriptionCount = 0u
	};

	// pipeline input-assembly state create-info
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	// viewport
	VkViewport viewport
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(m_SwapchainExtent.width),
		.height = static_cast<float>(m_SwapchainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	// siccors
	VkRect2D siccors
	{
		.offset = { 0, 0 },
		.extent = m_SwapchainExtent
	};

	// viewportState
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1u,
		.pViewports = &viewport,
		.scissorCount = 1u,
		.pScissors = &siccors
	};

	// pipeline rasterization state create-info
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f,
	};

	// pipeline multisample state create-info
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	// pipeline color blend attachment state
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	// pipeline color blend state create-info
	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1u,
		.pAttachments = &pipelineColorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
	};

	// dynamic state
	VkDynamicState dynamicState[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH,
	};

	// pipeline dynamic state create-info
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2u,
		.pDynamicStates = dynamicState
	};

	// pipeline create-info
	VkGraphicsPipelineCreateInfo pipelineCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = shaderStages.size(),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pipelineVertexStateCreateInfo,
		.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
		.pViewportState = &pipelineViewportStateCreateInfo,
		.pRasterizationState = &pipelineRasterizationStateCreateInfo,
		.pMultisampleState = &pipelineMultisampleStateCreateInfo,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &pipelineColorBlendStateCreateInfo,
		.pDynamicState = nullptr,
		.layout = m_PipelineLayout,
		.renderPass = m_RenderPass,
		.subpass = 0u,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	// create graphics pipelines
	VKC(vkCreateGraphicsPipelines(m_SharedContext.logicalDevice, VK_NULL_HANDLE, 1u, &pipelineCreateInfo, nullptr, &m_Pipeline));
}

void Pipeline::CreateFramebuffers()
{
	m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

	for (size_t i = 0ull; i < m_SwapchainImageViews.size(); i++)
	{
		// framebuffer create-info
		VkFramebufferCreateInfo framebufferCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_RenderPass,
			.attachmentCount = 1u,
			.pAttachments = &m_SwapchainImageViews[i],
			.width = m_SwapchainExtent.width,
			.height = m_SwapchainExtent.height,
			.layers = 1u
		};

		VKC(vkCreateFramebuffer(m_SharedContext.logicalDevice, &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffers[i]));
	}
}

void Pipeline::CreateCommandPool()
{
	// command pool create-info
	VkCommandPoolCreateInfo commandpoolCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = NULL,
		.queueFamilyIndex = m_SharedContext.queueFamilies.graphics.value(),
	};


	VKC(vkCreateCommandPool(m_SharedContext.logicalDevice, &commandpoolCreateInfo, nullptr, &m_CommandPool));
}

void Pipeline::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_SwapchainFramebuffers.size());

	// command buffer allocate-info
	VkCommandBufferAllocateInfo commandBufferAllocateInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size()),
	};

	VKC(vkAllocateCommandBuffers(m_SharedContext.logicalDevice, &commandBufferAllocateInfo, m_CommandBuffers.data()));

	for (size_t i = 0ull; i < m_CommandBuffers.size(); i++)
	{
		// command buffer begin-info
		VkCommandBufferBeginInfo  commandBufferBeginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = NULL,
			.pInheritanceInfo = nullptr,
		};

		VKC(vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo));

		// clear value
		VkClearValue clearColor =
		{
			.color = { 0.124f, 0.345f, 0.791f, 1.0f }
		};

		// render pass begin-info
		VkRenderPassBeginInfo  renderpassBeginInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_RenderPass,
			.framebuffer = m_SwapchainFramebuffers[i],

			.renderArea =
			{
				.offset = {0, 0},
				.extent = m_SwapchainExtent
			},

			.clearValueCount = 1u,
			.pClearValues = &clearColor
		};


		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(m_CommandBuffers[i]);

		VKC(vkEndCommandBuffer(m_CommandBuffers[i]));
	}

}

void Pipeline::CreateSemaphores()
{
	// semaphor create-info
	VkSemaphoreCreateInfo semaphoreCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	vkCreateSemaphore(m_SharedContext.logicalDevice, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphor);
	vkCreateSemaphore(m_SharedContext.logicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphor);
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
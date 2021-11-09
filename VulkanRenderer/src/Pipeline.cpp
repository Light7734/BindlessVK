#include "Pipeline.h"
#include "Shader.h"

#include <glfw/glfw3.h>

Pipeline::Pipeline(GLFWwindow* windowHandle, uint32_t frames /* = 2u */) :
	// window
	m_WindowHandle(windowHandle),
	m_Surface(VK_NULL_HANDLE),

	// device
	m_VkInstance(VK_NULL_HANDLE),
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_LogicalDevice(VK_NULL_HANDLE),

	// queues
	m_QueueFamilyIndices{},

	m_GraphicsQueue(VK_NULL_HANDLE),
	m_PresentQueue(VK_NULL_HANDLE),

	// validation layers & extensions
	m_ValidationLayers{ "VK_LAYER_KHRONOS_validation" },
	m_RequiredExtensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
	m_LogicalDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME },

	// pipeline
	m_Pipeline(VK_NULL_HANDLE),
	m_PipelineLayout(VK_NULL_HANDLE),
	m_RenderPass(VK_NULL_HANDLE),

	// swap chain
	m_Swapchain(VK_NULL_HANDLE),
	m_SwapchainImages{},
	m_SwapchainImageViews{},
	m_SwapchainFramebuffers{},
	m_SwapchainImageFormat(VK_FORMAT_UNDEFINED),
	m_SwapchainExtent{},

	m_SwapchainInvalidated(false),

	// command pool
	m_CommandPool(VK_NULL_HANDLE),
	m_CommandBuffers{},

	// synchronization
	m_FramesInFlight(frames),
	m_ImageAvailableSemaphores{},
	m_RenderFinishedSemaphores{},
	m_Fences{},
	m_ImagesInFlight{},
	m_CurrentFrame(0ull),

	m_ShaderTriangle{}
{
	// init vulkan
	VKC(volkInitialize());

	FilterValidationLayers();
	FetchRequiredExtensions();
	auto debugMessageCreateInfo = SetupDebugMessageCallback();

	// application info
	VkApplicationInfo appInfo
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

		.pApplicationName = "VulkanRenderer",
		.applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),

		.pEngineName = "",
		.engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),

		.apiVersion = VK_API_VERSION_1_2
	};

	// instance create-info
	VkInstanceCreateInfo instanceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &debugMessageCreateInfo,

		.pApplicationInfo = &appInfo,

		.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size()),
		.ppEnabledLayerNames = m_ValidationLayers.data(),

		.enabledExtensionCount = static_cast<uint32_t>(m_RequiredExtensions.size()),
		.ppEnabledExtensionNames = m_RequiredExtensions.data(),
	};

	// vulkan instance
	VKC(vkCreateInstance(&instanceCreateInfo, nullptr, &m_VkInstance));
	volkLoadInstance(m_VkInstance);

	// device context
	CreateWindowSurface(m_WindowHandle);
	PickPhysicalDevice();
	CreateLogicalDevice();
	FetchLogicalDeviceExtensions();

	// load shaders
	m_ShaderTriangle = std::make_unique<Shader>("res/VertexShader.glsl", "res/PixelShader.glsl", m_LogicalDevice);
	m_VertexBuffer = std::make_unique<Buffer>(m_DeviceContext, (sizeof(glm::vec3) + sizeof(glm::vec2)) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	// pipeline context
	FetchSwapchainSupportDetails();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreatePipelineLayout();
	CreatePipeline(m_ShaderTriangle->GetShaderStages());
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSynchronizations();
}

Pipeline::~Pipeline()
{
	// wait for drawing and presentation operations to end
	vkDeviceWaitIdle(m_LogicalDevice);

	// destroy shaders
	m_ShaderTriangle.reset();

	m_VertexBuffer.reset();

	// destroy swap chain
	DestroySwapchain();

	// destroy semaphores & fences
	for (uint32_t i = 0ull; i < m_FramesInFlight; i++)
	{
		vkDestroySemaphore(m_LogicalDevice, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_LogicalDevice, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_LogicalDevice, m_Fences[i], nullptr);
	}

	// destroy command pool
	vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);

	// destroy device
	vkDestroyDevice(m_LogicalDevice, nullptr);
	vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

void Pipeline::RenderFrame()
{
	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, { (1.0f + sin(glfwGetTime())) / 2.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f + (cos(glfwGetTime()) / 2.0f), 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, abs(tan(glfwGetTime()))}}
	};

	void* map = m_VertexBuffer->Map((sizeof(glm::vec3) + sizeof(glm::vec2)) * 3);
	memcpy(map, &vertices[0], (sizeof(glm::vec3) + sizeof(glm::vec2)) * 3);
	m_VertexBuffer->Unmap();

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS)
		throw vkException(result, __FILE__, __LINE__);

	if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(m_LogicalDevice, 1u, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	m_ImagesInFlight[imageIndex] = m_Fences[m_CurrentFrame];
	vkWaitForFences(m_LogicalDevice, 1u, &m_Fences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// submit info
	VkSubmitInfo submitInfo
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame],
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1u,
		.pCommandBuffers = &m_CommandBuffers[imageIndex],
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame],
	};

	// submit queue
	vkResetFences(m_LogicalDevice, 1u, &m_Fences[m_CurrentFrame]);
	VKC(vkQueueSubmit(m_GraphicsQueue, 1u, &submitInfo, m_Fences[m_CurrentFrame]));

	VkPresentInfoKHR presentInfo
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame],
		.swapchainCount = 1u,
		.pSwapchains = &m_Swapchain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,
	};

	m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapchain();
	}
	else if (result != VK_SUCCESS) {
		throw vkException(result, __FILE__, __LINE__);
	}
}

void Pipeline::PickPhysicalDevice()
{
	// fetch physical devices
	uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
	ASSERT(deviceCount, "GraphicsContext::PickPhysicalDevice: failed to find a GPU with vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

	// select most suitable physical device
	uint8_t highestDeviceScore = 0u;
	for (const auto& device : devices)
	{
		uint32_t deviceScore = 0u;

		// fetch properties & features
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		vkGetPhysicalDeviceProperties(device, &properties);
		vkGetPhysicalDeviceFeatures(device, &features);

		// geometry shader is needed for rendering
		if (!features.geometryShader)
			continue;

		// discrete gpu is favored
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			deviceScore += 10000u;

		deviceScore += properties.limits.maxImageDimension2D;

		// more suitable device found
		if (deviceScore > highestDeviceScore)
		{
			m_PhysicalDevice = device;

			// check if device supports required queue families
			FetchSupportedQueueFamilies();
			if (!m_QueueFamilyIndices)
				m_PhysicalDevice = VK_NULL_HANDLE;
			else
				highestDeviceScore = deviceScore;
		}
	}

	ASSERT(m_PhysicalDevice, "Pipeline::PickPhysicalDevice: failed to find suitable GPU for vulkan");

	m_DeviceContext.physical = m_PhysicalDevice;
}

void Pipeline::CreateLogicalDevice()
{
	// fetch properties & features
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	deviceQueueCreateInfos.reserve(m_QueueFamilyIndices.indices.size());

	// device queue create-infos
	float queuePriority = 1.0f;
	for (const auto& queueFamilyIndex : m_QueueFamilyIndices.indices)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,

			.queueFamilyIndex = queueFamilyIndex,
			.queueCount = 1u,

			.pQueuePriorities = &queuePriority,
		};

		deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	// device create-info
	VkDeviceCreateInfo deviceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

		.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
		.pQueueCreateInfos = deviceQueueCreateInfos.data(),

		.enabledExtensionCount = static_cast<uint32_t>(m_LogicalDeviceExtensions.size()),
		.ppEnabledExtensionNames = m_LogicalDeviceExtensions.data(),

		.pEnabledFeatures = &deviceFeatures,
	};

	// create logical device and get it's queues
	VKC(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice));
	vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.graphics.value(), 0u, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.present.value(), 0u, &m_PresentQueue);

	ASSERT(m_GraphicsQueue, "Pipeline::CreateLogicalDevice: failed to get graphics queue");
	ASSERT(m_PresentQueue, "Pipeline::CreateLogicalDevice: failed to get present queue");

	m_DeviceContext.logical = m_LogicalDevice;
}

void Pipeline::CreateWindowSurface(GLFWwindow* windowHandle)
{
	glfwCreateWindowSurface(m_VkInstance, windowHandle, nullptr, &m_Surface);
}

void Pipeline::CreateSwapchain()
{
	// pick the desired swap chain surface format
	VkSurfaceFormatKHR swapChainSurfaceFormat{};
	for (const auto& surfaceFormat : m_SwapChainDetails.formats)
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			swapChainSurfaceFormat = surfaceFormat;

	// pick the desired swap chain present mode
	VkPresentModeKHR swapChainPresentMode{};
	for (const auto& presentMode : m_SwapChainDetails.presentModes)
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			swapChainPresentMode = presentMode;

	// desired format/mode not found
	if (swapChainSurfaceFormat.format == VK_FORMAT_UNDEFINED)
		swapChainSurfaceFormat = m_SwapChainDetails.formats[0];

	if (swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
		swapChainPresentMode = m_SwapChainDetails.presentModes[0];

	// calculate swapchain extents
	if (m_SwapChainDetails.capabilities.currentExtent.width == UINT32_MAX)
	{
		// fetch window resolution
		int width, height;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);

		// set extents
		m_SwapchainExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		// clamp extents
		m_SwapchainExtent.width = std::clamp(m_SwapchainExtent.width, m_SwapChainDetails.capabilities.minImageExtent.width, m_SwapChainDetails.capabilities.maxImageExtent.width);
		m_SwapchainExtent.height = std::clamp(m_SwapchainExtent.height, m_SwapChainDetails.capabilities.minImageExtent.height, m_SwapChainDetails.capabilities.maxImageExtent.height);
	}
	else
		m_SwapchainExtent = m_SwapChainDetails.capabilities.currentExtent;

	// calculate swapchain image count 
	const uint32_t maxImageCount = m_SwapChainDetails.capabilities.maxImageCount;
	const uint32_t minImageCount = m_SwapChainDetails.capabilities.minImageCount;
	uint32_t imageCount = maxImageCount > 0 && minImageCount + 1 > maxImageCount ? maxImageCount : minImageCount + 1;

	bool queueFamiliesSameIndex = m_QueueFamilyIndices.graphics.value() == m_QueueFamilyIndices.present.value();

	// swapchain create-info
	VkSwapchainCreateInfoKHR swapchainCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,

		.surface = m_Surface,

		.minImageCount = imageCount,
		.imageFormat = swapChainSurfaceFormat.format,
		.imageColorSpace = swapChainSurfaceFormat.colorSpace,
		.imageExtent = m_SwapchainExtent,
		.imageArrayLayers = 1u,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = queueFamiliesSameIndex ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,

		.queueFamilyIndexCount = queueFamiliesSameIndex ? 0u : 2u,
		.pQueueFamilyIndices = queueFamiliesSameIndex ? nullptr : m_QueueFamilyIndices.indices.data(),
		.preTransform = m_SwapChainDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapChainPresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	// create swap chain
	VKC(vkCreateSwapchainKHR(m_LogicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain));

	// store image format
	m_SwapchainImageFormat = swapChainSurfaceFormat.format;

	// fetch images
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, m_SwapchainImages.data());
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
		VKC(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_SwapchainImageViews[i]));
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
	VKC(vkCreateRenderPass(m_LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass));

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
	VKC(vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
}

void Pipeline::CreatePipeline(std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages)
{
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributesDescription = Vertex::GetAttributesDescription();

	// pipeline Vertex state create-info
	VkPipelineVertexInputStateCreateInfo pipelineVertexStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

		.vertexBindingDescriptionCount = 1u,
		.pVertexBindingDescriptions = &bindingDescription,

		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributesDescription.size()),
		.pVertexAttributeDescriptions = attributesDescription.data()
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
	VKC(vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1u, &pipelineCreateInfo, nullptr, &m_Pipeline));
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

		VKC(vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffers[i]));
	}
}

void Pipeline::CreateCommandPool()
{
	// command pool create-info
	VkCommandPoolCreateInfo commandpoolCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = NULL,
		.queueFamilyIndex = m_QueueFamilyIndices.graphics.value(),
	};


	VKC(vkCreateCommandPool(m_LogicalDevice, &commandpoolCreateInfo, nullptr, &m_CommandPool));
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

	VKC(vkAllocateCommandBuffers(m_LogicalDevice, &commandBufferAllocateInfo, m_CommandBuffers.data()));

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

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0u, 1u, m_VertexBuffer->GetBuffer(), offsets);

		vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_CommandBuffers[i]);

		VKC(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void Pipeline::CreateSynchronizations()
{
	m_ImageAvailableSemaphores.resize(m_FramesInFlight);
	m_RenderFinishedSemaphores.resize(m_FramesInFlight);
	m_Fences.resize(m_FramesInFlight);
	m_ImagesInFlight.resize(m_SwapchainImages.size(), VK_NULL_HANDLE);

	// semaphor create-info
	VkSemaphoreCreateInfo semaphoreCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	// fence create-info
	VkFenceCreateInfo fenceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (uint32_t i = 0ull; i < m_FramesInFlight; i++)
	{
		VKC(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		VKC(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]));

		vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_Fences[i]);

	}
}

void Pipeline::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_LogicalDevice);

	FetchSwapchainSupportDetails();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreatePipeline(m_ShaderTriangle->GetShaderStages());
	CreatePipelineLayout();
	CreateFramebuffers();
	CreateCommandBuffers();
}

void Pipeline::DestroySwapchain()
{
	// destroy framebuffers
	for (auto framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);

	vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());

	// destroy pipeline
	vkDestroyPipeline(m_LogicalDevice, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);

	// destroy image views
	for (auto swapchainImageView : m_SwapchainImageViews)
		vkDestroyImageView(m_LogicalDevice, swapchainImageView, nullptr);

	// destroy swap chain
	vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, nullptr);
}

void Pipeline::FilterValidationLayers()
{
	// fetch available layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// remove requested layers that are not supported
	for (const char* layerName : m_ValidationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		// layer not found
		if (!layerFound)
		{
			m_ValidationLayers.erase(std::find(m_ValidationLayers.begin(), m_ValidationLayers.end(), layerName));
			LOG(err, "Pipeline::FilterValidationLayers: failed to find validation layer: {}", layerName);
		}
	}
}

void Pipeline::FetchRequiredExtensions()
{
	// fetch required global extensions
	uint32_t glfwExtensionsCount = 0u;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	// append glfw's required extensions to our required extensions
	m_RequiredExtensions.insert(m_RequiredExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionsCount);
}

void Pipeline::FetchLogicalDeviceExtensions()
{
	// fetch device extensions
	uint32_t deviceExtensionsCount = 0u;
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionsCount);
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, deviceExtensions.data());

	// check if device supports the required extensions
	std::set<std::string> requiredExtensions(m_LogicalDeviceExtensions.begin(), m_LogicalDeviceExtensions.end());

	for (const auto& deviceExtension : deviceExtensions)
		requiredExtensions.erase(std::string(deviceExtension.extensionName));

	if (!requiredExtensions.empty())
	{
		LOG(critical, "Pipeline::FetchDeviceExtensions: device does not supprot required extensions: ");

		for (auto requiredExtension : requiredExtensions)
			LOG(critical, "        {}", requiredExtension);

		ASSERT(false, "Pipeline::FetchDeviceExtensions: aforementioned device extensinos are not supported");
	}
}

void Pipeline::FetchSupportedQueueFamilies()
{
	m_QueueFamilyIndices = {};

	// fetch queue families
	uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

	// find queue family indices
	uint32_t index = 0u;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_QueueFamilyIndices.graphics = index;
			LOG(info, "Pipeline::FetchSupportedQueueFamilies: graphics index: {}", index);
		}

		VkBool32 hasPresentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, m_Surface, &hasPresentSupport);

		if (hasPresentSupport)
		{
			m_QueueFamilyIndices.present = index;
			LOG(info, "Pipeline::FetchSupportedQueueFamilies: present index: {}", index);
		}

		index++;
	}

	m_QueueFamilyIndices.indices = { m_QueueFamilyIndices.graphics.value(), m_QueueFamilyIndices.present.value() };
}

void Pipeline::FetchSwapchainSupportDetails()
{
	// fetch device surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SwapChainDetails.capabilities);

	// fetch device surface formats
	uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
	ASSERT(formatCount, "Pipeline::FetchSwapchainSupportDetails: no physical device surface format");

	m_SwapChainDetails.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, m_SwapChainDetails.formats.data());

	// fetch device surface present modes
	uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
	ASSERT(presentModeCount, "Pipeline::FetchSwapchainSupportDetails: no phyiscal device surface present mode");

	m_SwapChainDetails.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, m_SwapChainDetails.presentModes.data());
}

uint32_t Pipeline::FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &physicalMemoryProperties);

	for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (physicalMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}

	throw std::runtime_error("bruh moment right here");
	return -1;
}

VkDebugUtilsMessengerCreateInfoEXT Pipeline::SetupDebugMessageCallback()
{
	// debug messenger create-info
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pUserData = nullptr
	};

	// debug message callback
	debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		LOGVk(trace, "{}", pCallbackData->pMessage);
		return VK_FALSE;
	};

	return debugMessengerCreateInfo;
}
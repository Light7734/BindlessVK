#include "Graphics/Renderer.hpp"

#include "Utils/Timer.hpp"

Renderer::Renderer(const RendererCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_SurfaceInfo(createInfo.surfaceInfo), m_QueueInfo(createInfo.queueInfo), m_SampleCount(createInfo.sampleCount)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create sync objects
	{
		m_AquireImageSemaphores.resize(m_MaxFramesInFlight);
		m_RenderSemaphores.resize(m_MaxFramesInFlight);
		m_FrameFences.resize(m_MaxFramesInFlight);

		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};

			VkFenceCreateInfo fenceCreateInfo {
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			};

			VKC(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_AquireImageSemaphores[i]));
			VKC(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderSemaphores[i]));
			VKC(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_FrameFences[i]));
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the swap chain.
	{
		// Select extent

		// Select image count ; one more than minImageCount, if minImageCount +1 is not higher than maxImageCount (maxImageCount of 0 means no limit)
		uint32_t imageCount = m_SurfaceInfo.capabilities.maxImageCount > 0 && m_SurfaceInfo.capabilities.minImageCount + 1 > m_SurfaceInfo.capabilities.maxImageCount ?
		                          m_SurfaceInfo.capabilities.maxImageCount :    // TRUE
		                          m_SurfaceInfo.capabilities.minImageCount + 1; // FALSE

		// Create swapchain
		bool sameQueueIndex = m_QueueInfo.graphicsQueueIndex == m_QueueInfo.presentQueueIndex;
		VkSwapchainCreateInfoKHR swapchainCreateInfo {
			.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface               = m_SurfaceInfo.surface,
			.minImageCount         = imageCount,
			.imageFormat           = m_SurfaceInfo.format.format,
			.imageColorSpace       = m_SurfaceInfo.format.colorSpace,
			.imageExtent           = m_SurfaceInfo.capabilities.currentExtent,
			.imageArrayLayers      = 1u,
			.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Write directly to the image (we would use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT if we wanted to do post-processing)
			.imageSharingMode      = sameQueueIndex ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = sameQueueIndex ? 0u : 2u,
			.pQueueFamilyIndices   = sameQueueIndex ? nullptr : &m_QueueInfo.graphicsQueueIndex,
			.preTransform          = m_SurfaceInfo.capabilities.currentTransform,
			.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // No alpha-blending between multiple windows
			.presentMode           = m_SurfaceInfo.presentMode,
			.clipped               = VK_TRUE, // Don't render the obsecured pixels
			.oldSwapchain          = VK_NULL_HANDLE,
		};

		VKC(vkCreateSwapchainKHR(m_LogicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch swap chain images and create image views
	{
		// Fetch images
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		m_ImageViews.resize(imageCount);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, m_Images.data());

		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo {
				.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image      = m_Images[i],
				.viewType   = VK_IMAGE_VIEW_TYPE_2D,
				.format     = m_SurfaceInfo.format.format,
				.components = {
				    // Don't swizzle the colors around...
				    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
				    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // Image will be used as color target
				    .baseMipLevel   = 0,                         // No mipmaipping
				    .levelCount     = 1,                         // No levels
				    .baseArrayLayer = 0,                         // No nothin...
				    .layerCount     = 1,
				},
			};

			VKC(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_ImageViews[i]));
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create color image
	{
		VkImageCreateInfo imageCreateInfo {
			.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags     = 0x0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format    = m_SurfaceInfo.format.format,
			.extent    = {
			       .width  = m_SurfaceInfo.capabilities.currentExtent.width,
			       .height = m_SurfaceInfo.capabilities.currentExtent.height,
			       .depth  = 1u,
            },
			.mipLevels     = 1u,
			.arrayLayers   = 1u,
			.samples       = m_SampleCount,
			.tiling        = VK_IMAGE_TILING_OPTIMAL,
			.usage         = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VKC(vkCreateImage(m_LogicalDevice, &imageCreateInfo, nullptr, &m_ColorImage));

		/* Alloacte and bind the color image memory */
		{
			// Fetch memory requirements
			VkMemoryRequirements imageMemReq;
			vkGetImageMemoryRequirements(m_LogicalDevice, m_ColorImage, &imageMemReq);

			// Fetch device memory properties
			VkPhysicalDeviceMemoryProperties physicalMemProps;
			vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);

			// Find adequate memory indices
			uint32_t imageMemTypeIndex = UINT32_MAX;

			for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
			{
				if (imageMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				{
					imageMemTypeIndex = i;
				}
			}

			ASSERT(imageMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

			// Alloacte memory
			VkMemoryAllocateInfo imageMemAllocInfo {
				.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize  = imageMemReq.size,
				.memoryTypeIndex = imageMemTypeIndex,
			};

			// Bind memory
			VKC(vkAllocateMemory(m_LogicalDevice, &imageMemAllocInfo, nullptr, &m_ColorImageMemory));
			VKC(vkBindImageMemory(m_LogicalDevice, m_ColorImage, m_ColorImageMemory, 0u));
		}
		// Create color image-view
		VkImageViewCreateInfo imageViewCreateInfo {
			.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image      = m_ColorImage,
			.viewType   = VK_IMAGE_VIEW_TYPE_2D,
			.format     = m_SurfaceInfo.format.format,
			.components = {
			    // Don't swizzle the colors around...
			    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
			    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			    .baseMipLevel   = 0u,
			    .levelCount     = 1u,
			    .baseArrayLayer = 0u,
			    .layerCount     = 1u,
			},
		};

		VKC(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_ColorImageView));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create depth buffer
	{
		// Find depth format
		m_DepthFormat            = VK_FORMAT_UNDEFINED;
		bool hasStencilComponent = false;
		VkFormatProperties formatProperties;
		for (VkFormat format : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT })
		{
			vkGetPhysicalDeviceFormatProperties(createInfo.physicalDevice, format, &formatProperties);
			if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				m_DepthFormat       = format;
				hasStencilComponent = format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
			}
		}

		// Create depth image
		VkImageCreateInfo imageCreateInfo {
			.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags     = 0x0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format    = m_DepthFormat,
			.extent    = {
			       .width  = m_SurfaceInfo.capabilities.currentExtent.width,
			       .height = m_SurfaceInfo.capabilities.currentExtent.height,
			       .depth  = 1u,
            },
			.mipLevels     = 1u,
			.arrayLayers   = 1u,
			.samples       = m_SampleCount,
			.tiling        = VK_IMAGE_TILING_OPTIMAL,
			.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VKC(vkCreateImage(m_LogicalDevice, &imageCreateInfo, nullptr, &m_DepthImage));

		/* Alloacte and bind the depth image memory */
		{
			// Fetch memory requirements
			VkMemoryRequirements imageMemReq;
			vkGetImageMemoryRequirements(m_LogicalDevice, m_DepthImage, &imageMemReq);

			// Fetch device memory properties
			VkPhysicalDeviceMemoryProperties physicalMemProps;
			vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);

			// Find adequate memory indices
			uint32_t imageMemTypeIndex = UINT32_MAX;

			for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
			{
				if (imageMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				{
					imageMemTypeIndex = i;
				}
			}

			ASSERT(imageMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

			// Alloacte memory
			VkMemoryAllocateInfo imageMemAllocInfo {
				.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize  = imageMemReq.size,
				.memoryTypeIndex = imageMemTypeIndex,
			};

			// Bind memory
			VKC(vkAllocateMemory(m_LogicalDevice, &imageMemAllocInfo, nullptr, &m_DepthImageMemory));
			VKC(vkBindImageMemory(m_LogicalDevice, m_DepthImage, m_DepthImageMemory, 0u));
		}
		// Create depth image-view
		VkImageViewCreateInfo imageViewCreateInfo {
			.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image      = m_DepthImage,
			.viewType   = VK_IMAGE_VIEW_TYPE_2D,
			.format     = m_DepthFormat,
			.components = {
			    // Don't swizzle the colors around...
			    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
			    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
			    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
			    .baseMipLevel   = 0u,
			    .levelCount     = 1u,
			    .baseArrayLayer = 0u,
			    .layerCount     = 1u,
			},
		};

		VKC(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_DepthImageView));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Specify the attachments and subpasses and create the renderpass
	{
		// Attachments
		std::vector<VkAttachmentDescription> attachments;
		attachments.push_back(VkAttachmentDescription {
		    .format         = m_SurfaceInfo.format.format,
		    .samples        = m_SampleCount,
		    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		    .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		});

		attachments.push_back(VkAttachmentDescription {
		    .format         = m_DepthFormat,
		    .samples        = m_SampleCount,
		    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		});

		attachments.push_back(VkAttachmentDescription {
		    .format         = m_SurfaceInfo.format.format,
		    .samples        = VK_SAMPLE_COUNT_1_BIT,
		    .loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		});

		// Subpass
		VkAttachmentReference colorAttachmentRef {
			.attachment = 0u,
			.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depthAttachmentRef {
			.attachment = 1u,
			.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colorAttachmentResolveRef {
			.attachment = 2u,
			.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpassDesc {
			.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount    = 1u,
			.pColorAttachments       = &colorAttachmentRef,
			.pResolveAttachments     = &colorAttachmentResolveRef,
			.pDepthStencilAttachment = &depthAttachmentRef,
		};

		// Subpass dependency
		VkSubpassDependency subpassDependency {
			.srcSubpass    = VK_SUBPASS_EXTERNAL,
			.dstSubpass    = 0u,
			.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0u,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		// Renderpass
		VkRenderPassCreateInfo renderPassCreateInfo {
			.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments    = attachments.data(),
			.subpassCount    = 1u,
			.pSubpasses      = &subpassDesc,
			.dependencyCount = 1u,
			.pDependencies   = &subpassDependency,
		};

		VKC(vkCreateRenderPass(m_LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the framebuffers
	{
		m_Framebuffers.resize(m_Images.size());
		for (uint32_t i = 0; i < m_Framebuffers.size(); i++)
		{
			std::vector<VkImageView> imageViews = { m_ColorImageView, m_DepthImageView, m_ImageViews[i] };
			VkFramebufferCreateInfo framebufferCreateInfo {
				.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass      = m_RenderPass,
				.attachmentCount = static_cast<uint32_t>(imageViews.size()),
				.pAttachments    = imageViews.data(),
				.width           = m_SurfaceInfo.capabilities.currentExtent.width,
				.height          = m_SurfaceInfo.capabilities.currentExtent.height,
				.layers          = 1u,
			};

			VKC(vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &m_Framebuffers[i]));
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the command pool
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = m_QueueInfo.graphicsQueueIndex,
		};

		VKC(vkCreateCommandPool(m_LogicalDevice, &commandPoolCreateInfo, nullptr, &m_CommandPool));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create uniform buffers
	{
		BufferCreateInfo mvpBufferCreateInfo {
			.logicalDevice  = m_LogicalDevice,
			.physicalDevice = createInfo.physicalDevice,
			.commandPool    = m_CommandPool,
			.graphicsQueue  = m_QueueInfo.graphicsQueue,
			.usage          = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.size           = sizeof(UniformMVP),
		};

		m_MVPUniBuffer.resize(m_MaxFramesInFlight);
		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			m_MVPUniBuffer[i] = std::make_shared<Buffer>(mvpBufferCreateInfo);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the texture
	{
		TextureCreateInfo textureCreateInfo {
			.logicalDevice     = m_LogicalDevice,
			.physicalDevice    = createInfo.physicalDevice,
			.graphicsQueue     = m_QueueInfo.graphicsQueue,
			.commandPool       = m_CommandPool,
			.imagePath         = "VulkanRenderer/res/viking_room.png",
			.anisotropyEnabled = VK_TRUE,
			.maxAnisotropy     = createInfo.physicalDeviceProperties.limits.maxSamplerAnisotropy,
		};

		m_StatueTexture = std::make_shared<Texture>(textureCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create descriptor pool
	{
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20 },
		};

		descriptorPoolSizes.push_back(VkDescriptorPoolSize {});

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
			.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets       = m_MaxFramesInFlight,
			.poolSizeCount = 3u,
			.pPoolSizes    = descriptorPoolSizes.data(),
		};

		VKC(vkCreateDescriptorPool(m_LogicalDevice, &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create pipelines
	{
		// Update model view projection uniform
		UniformMVP mvp;
		mvp.model = glm::rotate(glm::mat4(1.0f), std::cos(1.0f) * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.view  = glm::lookAt(glm::vec3(4.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		mvp.proj  = glm::perspective(glm::radians(45.0f), m_SurfaceInfo.capabilities.currentExtent.width / (float)m_SurfaceInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

		void* mvpMap = m_MVPUniBuffer[m_CurrentFrame]->Map();
		memcpy(mvpMap, &mvp, sizeof(UniformMVP));
		m_MVPUniBuffer[m_CurrentFrame]->Unmap();

		RenderableCreateInfo renderableCreateInfo {
			.modelPath = "VulkanRenderer/res/viking_room.obj",
			.textures  = { m_StatueTexture } // #TODO
		};

		PipelineCreateInfo pipelineCreateInfo {
			.logicalDevice         = m_LogicalDevice,
			.physicalDevice        = createInfo.physicalDevice,
			.maxFramesInFlight     = m_MaxFramesInFlight,
			.queueInfo             = m_QueueInfo,
			.viewportExtent        = m_SurfaceInfo.capabilities.currentExtent,
			.commandPool           = m_CommandPool,
			.imageCount            = static_cast<uint32_t>(m_Images.size()),
			.sampleCount           = createInfo.sampleCount,
			.renderPass            = m_RenderPass,
			.viewProjectionBuffers = m_MVPUniBuffer,
			.descriptorPool        = m_DescriptorPool,

			// Shader
			.vertexShaderPath = "VulkanRenderer/res/vertex.glsl",
			.pixelShaderPath  = "VulkanRenderer/res/pixel.glsl",

			// Vertex input
			.vertexBindingDescs = {
			    VkVertexInputBindingDescription {
			        .binding   = 0u,
			        .stride    = sizeof(Vertex),
			        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			    },
			},
			.vertexAttribDescs = {
			    VkVertexInputAttributeDescription {
			        .location = 0u,
			        .binding  = 0u,
			        .format   = VK_FORMAT_R32G32B32_SFLOAT,
			        .offset   = 0u,
			    },
			    VkVertexInputAttributeDescription {
			        .location = 1u,
			        .binding  = 0u,
			        .format   = VK_FORMAT_R32G32B32_SFLOAT,
			        .offset   = sizeof(glm::vec3),
			    },
			    VkVertexInputAttributeDescription {
			        .location = 2u,
			        .binding  = 0u,
			        .format   = VK_FORMAT_R32G32_SFLOAT,
			        .offset   = sizeof(glm::vec3) + sizeof(glm::vec3),
			    },
			    VkVertexInputAttributeDescription {
			        .location = 3u,
			        .binding  = 0u,
			        .format   = VK_FORMAT_R32_UINT,
			        .offset   = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2),
			    },
			},
		};

		m_Pipelines.push_back(std::make_shared<Pipeline>(pipelineCreateInfo));
		m_Pipelines.back()->CreateRenderable(renderableCreateInfo);
		m_Pipelines.back()->RecreateBuffers();
	}
}

void Renderer::BeginFrame()
{
	LOG(critical, "Not implemented!");
}

void Renderer::Draw()
{
	LOG(critical, "Not implemented!");
}

void Renderer::EndFrame()
{
	// Timer...
	static Timer timer;
	float time = timer.ElapsedTime();

	// Wait for the frame fence
	VKC(vkWaitForFences(m_LogicalDevice, 1u, &m_FrameFences[m_CurrentFrame], VK_TRUE, UINT64_MAX));

	// Acquire an image
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_Swapchain, UINT64_MAX, m_AquireImageSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_SwapchainInvalidated)
	{
		VKC(vkDeviceWaitIdle(m_LogicalDevice));
		m_SwapchainInvalidated = true;
		return;
	}
	else
	{
		ASSERT(result == VK_SUCCESS, "VkAcquireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR");
	}

	// Update model view projection uniform
	UniformMVP mvp;
	mvp.model = glm::rotate(glm::mat4(1.0f), std::cos(time) * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	mvp.view  = glm::lookAt(glm::vec3(4.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	mvp.proj  = glm::perspective(glm::radians(45.0f), m_SurfaceInfo.capabilities.currentExtent.width / (float)m_SurfaceInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

	void* mvpMap = m_MVPUniBuffer[m_CurrentFrame]->Map();
	memcpy(mvpMap, &mvp, sizeof(UniformMVP));
	m_MVPUniBuffer[m_CurrentFrame]->Unmap();

	// Record commands
	CommandBufferStartInfo commandBufferStartInfo {
		.descriptorSet = &m_DescriptorSets[m_CurrentFrame],
		.framebuffer   = m_Framebuffers[imageIndex],
		.extent        = m_SurfaceInfo.capabilities.currentExtent,
		.frameIndex    = m_CurrentFrame,
	};

	std::vector<VkCommandBuffer> cmdBuffers = {};
	for (auto& pipeline : m_Pipelines)
	{
		cmdBuffers.push_back(pipeline->RecordCommandBuffer(commandBufferStartInfo));
	}

	// Submit commands (and reset the fence)
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo {
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount   = 1u,
		.pWaitSemaphores      = &m_AquireImageSemaphores[m_CurrentFrame],
		.pWaitDstStageMask    = &waitStage,
		.commandBufferCount   = static_cast<uint32_t>(cmdBuffers.size()),
		.pCommandBuffers      = cmdBuffers.data(),
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores    = &m_RenderSemaphores[m_CurrentFrame],
	};

	VKC(vkResetFences(m_LogicalDevice, 1u, &m_FrameFences[m_CurrentFrame]));
	VKC(vkQueueSubmit(m_QueueInfo.graphicsQueue, 1u, &submitInfo, m_FrameFences[m_CurrentFrame]));

	// Present the image
	VkPresentInfoKHR presentInfo {
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores    = &m_RenderSemaphores[m_CurrentFrame],
		.swapchainCount     = 1u,
		.pSwapchains        = &m_Swapchain,
		.pImageIndices      = &imageIndex,
		.pResults           = nullptr
	};

	vkQueuePresentKHR(m_QueueInfo.presentQueue, &presentInfo);

	// Increment frame index
	m_CurrentFrame = (m_CurrentFrame + 1u) % m_MaxFramesInFlight;
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_LogicalDevice);

	for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
	{
		vkDestroySemaphore(m_LogicalDevice, m_AquireImageSemaphores[i], nullptr);
		vkDestroySemaphore(m_LogicalDevice, m_RenderSemaphores[i], nullptr);
		vkDestroyFence(m_LogicalDevice, m_FrameFences[i], nullptr);
	}

	vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, nullptr);
	for (uint32_t i = 0; i < m_Images.size(); i++)
	{
		vkDestroyImageView(m_LogicalDevice, m_ImageViews[i], nullptr);
		vkDestroyFramebuffer(m_LogicalDevice, m_Framebuffers[i], nullptr);
	}

	vkDestroyImageView(m_LogicalDevice, m_DepthImageView, nullptr);
	vkDestroyImageView(m_LogicalDevice, m_ColorImageView, nullptr);
	vkDestroyImage(m_LogicalDevice, m_ColorImage, nullptr);
	vkDestroyImage(m_LogicalDevice, m_DepthImage, nullptr);
	vkFreeMemory(m_LogicalDevice, m_DepthImageMemory, nullptr);
	vkFreeMemory(m_LogicalDevice, m_ColorImageMemory, nullptr);

	vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
	vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
	vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);

	m_MVPUniBuffer.clear();
	vkDestroyDescriptorSetLayout(m_LogicalDevice, m_DescriptorSetLayout, nullptr);
}

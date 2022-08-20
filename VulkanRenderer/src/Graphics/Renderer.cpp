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
			vk::SemaphoreCreateInfo semaphoreCreateInfo {};

			vk::FenceCreateInfo fenceCreateInfo {
				vk::FenceCreateFlagBits::eSignaled, // flags
			};

			m_AquireImageSemaphores[i] = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
			m_RenderSemaphores[i]      = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
			m_FrameFences[i]           = m_LogicalDevice.createFence(fenceCreateInfo, nullptr);
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
		vk::SwapchainCreateInfoKHR swapchainCreateInfo {
			{},                                                                          // flags
			m_SurfaceInfo.surface,                                                       // surface
			imageCount,                                                                  // minImageCount
			m_SurfaceInfo.format.format,                                                 // imageFormat
			m_SurfaceInfo.format.colorSpace,                                             // imageColorSpace
			m_SurfaceInfo.capabilities.currentExtent,                                    // imageExtent
			1u,                                                                          // imageArrayLayers
			vk::ImageUsageFlagBits::eColorAttachment,                                    // Write directly to the image (we would use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT if we wanted to do post-processing) // imageUsage
			sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
			sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
			sameQueueIndex ? nullptr : &m_QueueInfo.graphicsQueueIndex,                  // pQueueFamilyIndices
			m_SurfaceInfo.capabilities.currentTransform,                                 // preTransform
			vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // No alpha-blending between multiple windows // compositeAlpha
			m_SurfaceInfo.presentMode,                                                   // presentMode
			VK_TRUE,                                                                     // Don't render the obsecured pixels // clipped
			VK_NULL_HANDLE,                                                              // oldSwapchain
		};

		m_Swapchain = m_LogicalDevice.createSwapchainKHR(swapchainCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch swap chain images and create image views
	{
		// Fetch images
		m_Images = m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain);
		m_ImageViews.resize(m_Images.size());

		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo {
				{},                          // flags
				m_Images[i],                 // image
				vk::ImageViewType::e2D,      // viewType
				m_SurfaceInfo.format.format, // format

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
				    vk::ImageAspectFlagBits::eColor, // Image will be used as color target // aspectMask
				    0,                               // No mipmaipping // baseMipLevel
				    1,                               // No levels // levelCount
				    0,                               // No nothin... // baseArrayLayer
				    1,                               // layerCount
				},
			};

			m_ImageViews[i] = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create color image
	{
		vk::ImageCreateInfo imageCreateInfo {
			{},                          // flags
			vk::ImageType::e2D,          // imageType
			m_SurfaceInfo.format.format, // format

			/* extent */

			vk::Extent3D {
			    m_SurfaceInfo.capabilities.currentExtent.width,  // width
			    m_SurfaceInfo.capabilities.currentExtent.height, // height
			    1u,                                              // depth
			},
			1u,                                                                                      // mipLevels
			1u,                                                                                      // arrayLayers
			m_SampleCount,                                                                           // samples
			vk::ImageTiling::eOptimal,                                                               // tiling
			vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, // usage
			vk::SharingMode::eExclusive,                                                             // sharingMode
			0u,                                                                                      // queueFamilyIndexCount
			nullptr,                                                                                 // pQueueFamilyIndices
			vk::ImageLayout::eUndefined,                                                             // initialLayout
		};

		m_ColorImage = m_LogicalDevice.createImage(imageCreateInfo, nullptr);

		/* Alloacte and bind the color image memory */
		{
			// Fetch memory requirements
			vk::MemoryRequirements imageMemReq = m_LogicalDevice.getImageMemoryRequirements(m_ColorImage);

			// Fetch device memory properties
			vk::PhysicalDeviceMemoryProperties physicalMemProps = createInfo.physicalDevice.getMemoryProperties();

			// Find adequate memory indices
			uint32_t imageMemTypeIndex = UINT32_MAX;

			for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
			{
				if (imageMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eDeviceLocal))
				{
					imageMemTypeIndex = i;
				}
			}

			ASSERT(imageMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

			// Alloacte memory
			vk::MemoryAllocateInfo imageMemAllocInfo {
				imageMemReq.size,  // allocationSize
				imageMemTypeIndex, // memoryTypeIndex
			};

			// Bind memory
			m_ColorImageMemory = m_LogicalDevice.allocateMemory(imageMemAllocInfo, nullptr);
			m_LogicalDevice.bindImageMemory(m_ColorImage, m_ColorImageMemory, {});
		}
		// Create color image-view
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                          // flags
			m_ColorImage,                // image
			vk::ImageViewType::e2D,      // viewType
			m_SurfaceInfo.format.format, // format

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
			    vk::ImageAspectFlagBits::eColor, // aspectMask
			    0u,                              // baseMipLevel
			    1u,                              // levelCount
			    0u,                              // baseArrayLayer
			    1u,                              // layerCount
			},
		};
		m_ColorImageView = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create depth buffer
	{
		// Find depth format
		m_DepthFormat            = vk::Format::eUndefined;
		bool hasStencilComponent = false;
		vk::FormatProperties formatProperties;
		for (vk::Format format : { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint })
		{
			formatProperties = createInfo.physicalDevice.getFormatProperties(format);
			if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				m_DepthFormat       = format;
				hasStencilComponent = format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
			}
		}

		// Create depth image
		vk::ImageCreateInfo imageCreateInfo {
			{},                 // flags
			vk::ImageType::e2D, // imageType
			m_DepthFormat,      // format

			/* extent */
			vk::Extent3D {
			    m_SurfaceInfo.capabilities.currentExtent.width,  // width
			    m_SurfaceInfo.capabilities.currentExtent.height, // height
			    1u,                                              // depth
			},
			1u,                                              // mipLevels
			1u,                                              // arrayLayers
			m_SampleCount,                                   // samples
			vk::ImageTiling::eOptimal,                       // tiling
			vk::ImageUsageFlagBits::eDepthStencilAttachment, // usage
			vk::SharingMode::eExclusive,                     // sharingMode
			0u,                                              // queueFamilyIndexCount
			nullptr,                                         // pQueueFamilyIndices
			vk::ImageLayout::eUndefined,                     // initialLayout
		};

		m_DepthImage = m_LogicalDevice.createImage(imageCreateInfo, nullptr);

		/* Alloacte and bind the depth image memory */
		{
			// Fetch memory requirements
			vk::MemoryRequirements imageMemReq = m_LogicalDevice.getImageMemoryRequirements(m_DepthImage);

			// Fetch device memory properties
			vk::PhysicalDeviceMemoryProperties physicalMemProps = createInfo.physicalDevice.getMemoryProperties();

			// Find adequate memory indices
			uint32_t imageMemTypeIndex = UINT32_MAX;

			for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
			{
				if (imageMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eDeviceLocal))
				{
					imageMemTypeIndex = i;
				}
			}

			ASSERT(imageMemTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

			// Alloacte memory
			vk::MemoryAllocateInfo imageMemAllocInfo {
				imageMemReq.size,  // allocationSize
				imageMemTypeIndex, // memoryTypeIndex
			};

			// Bind memory
			m_DepthImageMemory = m_LogicalDevice.allocateMemory(imageMemAllocInfo, nullptr);
			m_LogicalDevice.bindImageMemory(m_DepthImage, m_DepthImageMemory, {});
		}
		// Create depth image-view
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                     // flags
			m_DepthImage,           // image
			vk::ImageViewType::e2D, // viewType
			m_DepthFormat,          // format
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
			    vk::ImageAspectFlagBits::eDepth, // aspectMask
			    0u,                              // baseMipLevel
			    1u,                              // levelCount
			    0u,                              // baseArrayLayer
			    1u,                              // layerCount
			},
		};

		m_DepthImageView = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Specify the attachments and subpasses and create the renderpass
	{
		// Attachments
		std::vector<vk::AttachmentDescription> attachments;
		attachments.push_back({
		    {},                                       // flags
		    m_SurfaceInfo.format.format,              // format
		    m_SampleCount,                            // samples
		    vk::AttachmentLoadOp::eClear,             // loadOp
		    vk::AttachmentStoreOp::eStore,            // storeOp
		    vk::AttachmentLoadOp::eDontCare,          // stencilLoadOp
		    vk::AttachmentStoreOp::eDontCare,         // stencilStoreOp
		    vk::ImageLayout::eUndefined,              // initialLayout
		    vk::ImageLayout::eColorAttachmentOptimal, // finalLayout
		});

		attachments.push_back({
		    {},                                              // flags
		    m_DepthFormat,                                   // format
		    m_SampleCount,                                   // samples
		    vk::AttachmentLoadOp::eClear,                    // loadOp
		    vk::AttachmentStoreOp::eStore,                   // storeOp
		    vk::AttachmentLoadOp::eDontCare,                 // stencilLoadOp
		    vk::AttachmentStoreOp::eDontCare,                // stencilStoreOp
		    vk::ImageLayout::eUndefined,                     // initialLayout
		    vk::ImageLayout::eDepthStencilAttachmentOptimal, // finalLayout
		});


		attachments.push_back({
		    {},                               // flags
		    m_SurfaceInfo.format.format,      // format
		    vk::SampleCountFlagBits::e1,      // samples
		    vk::AttachmentLoadOp::eDontCare,  // loadOp
		    vk::AttachmentStoreOp::eStore,    // storeOp
		    vk::AttachmentLoadOp::eDontCare,  // stencilLoadOp
		    vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
		    vk::ImageLayout::eUndefined,      // initialLayout
		    vk::ImageLayout::ePresentSrcKHR,  // finalLayout
		});

		// Subpass
		vk::AttachmentReference colorAttachmentRef {
			0u,                                       // attachment
			vk::ImageLayout::eColorAttachmentOptimal, // layout
		};

		vk::AttachmentReference depthAttachmentRef {
			1u,                                              // attachment
			vk::ImageLayout::eDepthStencilAttachmentOptimal, // layout
		};

		vk::AttachmentReference colorAttachmentResolveRef {
			2u,                                       // attachment
			vk::ImageLayout::eColorAttachmentOptimal, // layout
		};

		vk::SubpassDescription subpassDesc {
			{},                               // flags
			vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
			0u,                               // ipnutAttachmentCount
			nullptr,                          // pInputAttachments
			1u,                               // colorAttachmentCount
			&colorAttachmentRef,              // pColorAttachments
			&colorAttachmentResolveRef,       // pResolveAttachments
			&depthAttachmentRef,              // pDepthStencilAttachment
		};

		// Subpass dependency
		vk::SubpassDependency subpassDependency {
			VK_SUBPASS_EXTERNAL,                                                                                // srcSubpass
			0u,                                                                                                 // dstSubpass
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, // srcStageMask
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, // dstStageMask
			{},                                                                                                 // srcAccessMask
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,       // dstAccessMask
			{},                                                                                                 // dependencytFlags
		};

		// Renderpass
		vk::RenderPassCreateInfo renderPassCreateInfo {
			{},                                        // flags
			static_cast<uint32_t>(attachments.size()), // attachmentCount
			attachments.data(),                        // pAttachments
			1u,                                        // subpassCount
			&subpassDesc,                              // pSubpasses
			1u,                                        // dependencyCount
			&subpassDependency,                        // pDependencies
		};

		m_RenderPass = m_LogicalDevice.createRenderPass(renderPassCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the framebuffers
	{
		m_Framebuffers.resize(m_Images.size());
		for (uint32_t i = 0; i < m_Framebuffers.size(); i++)
		{
			std::vector<vk::ImageView> imageViews = { m_ColorImageView, m_DepthImageView, m_ImageViews[i] };
			vk::FramebufferCreateInfo framebufferCreateInfo {
				{},                                              //flags
				m_RenderPass,                                    // renderPass
				static_cast<uint32_t>(imageViews.size()),        // attachmentCount
				imageViews.data(),                               // pAttachments
				m_SurfaceInfo.capabilities.currentExtent.width,  // width
				m_SurfaceInfo.capabilities.currentExtent.height, // height
				1u,                                              // layers
			};

			m_Framebuffers[i] = m_LogicalDevice.createFramebuffer(framebufferCreateInfo, nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the command pool
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
			m_QueueInfo.graphicsQueueIndex,                     // queueFamilyIndex
		};

		m_CommandPool = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
	}

	// Create the texture
	{
		TextureCreateInfo textureCreateInfo {
			m_LogicalDevice,                                                 // logicalDevice
			createInfo.physicalDevice,                                       // physicalDevice
			m_QueueInfo.graphicsQueue,                                       // graphicsQueue
			m_CommandPool,                                                   // commandPool
			"VulkanRenderer/res/viking_room.png",                            // imagePath
			VK_TRUE,                                                         // anisotropyEnabled
			createInfo.physicalDeviceProperties.limits.maxSamplerAnisotropy, // maxAnisotropy
		};

		m_StatueTexture = std::make_shared<Texture>(textureCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create descriptor pool
	{
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
			{ vk::DescriptorType::eUniformBuffer, 20 },
			{ vk::DescriptorType::eCombinedImageSampler, 20 },
			{ vk::DescriptorType::eStorageBuffer, 20 },
		};

		descriptorPoolSizes.push_back(VkDescriptorPoolSize {});

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {
			{},                         // flags
			m_MaxFramesInFlight,        // maxSets
			3u,                         // poolSizeCount
			descriptorPoolSizes.data(), // pPoolSizes
		};

		m_DescriptorPool = m_LogicalDevice.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create pipelines
	{
		// Update model view projection uniform
		m_ViewProjection.view       = glm::lookAt(glm::vec3(4.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		m_ViewProjection.projection = glm::perspective(glm::radians(45.0f), m_SurfaceInfo.capabilities.currentExtent.width / (float)m_SurfaceInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

		RenderableCreateInfo renderableCreateInfo {
			"VulkanRenderer/res/viking_room.obj", // modelPath
			{ m_StatueTexture }                   // #TODO // textures
		};

		// #TODO:
		PipelineCreateInfo pipelineCreateInfo {
			.logicalDevice     = m_LogicalDevice,
			.physicalDevice    = createInfo.physicalDevice,
			.maxFramesInFlight = m_MaxFramesInFlight,
			.queueInfo         = m_QueueInfo,
			.viewportExtent    = m_SurfaceInfo.capabilities.currentExtent,
			.commandPool       = m_CommandPool,
			.imageCount        = static_cast<uint32_t>(m_Images.size()),
			.sampleCount       = createInfo.sampleCount,
			.renderPass        = m_RenderPass,
			.descriptorPool    = m_DescriptorPool,

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
	if (m_SwapchainInvalidated)
		return;

	// Timer...
	static Timer timer;
	float time = timer.ElapsedTime();

	// Wait for the frame fence
	VKC(m_LogicalDevice.waitForFences(1u, &m_FrameFences[m_CurrentFrame], VK_TRUE, UINT64_MAX));

	// Acquire an image
	uint32_t imageIndex;
	vk::Result result = m_LogicalDevice.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_AquireImageSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
	{
		m_LogicalDevice.waitIdle();
		m_SwapchainInvalidated = true;
		return;
	}
	else
	{
		ASSERT(result == vk::Result::eSuccess, "VkAcquireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR");
	}

	// Update model view projection uniform
	m_ViewProjection.view       = glm::lookAt(glm::vec3(4.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_ViewProjection.projection = glm::perspective(glm::radians(45.0f), m_SurfaceInfo.capabilities.currentExtent.width / (float)m_SurfaceInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

	// Record commands
	CommandBufferStartInfo commandBufferStartInfo {
		&m_DescriptorSets[m_CurrentFrame],        // descriptorSet
		m_Framebuffers[imageIndex],               // framebuffer
		m_SurfaceInfo.capabilities.currentExtent, // extent
		m_CurrentFrame,                           // frameIndex
		&m_ViewProjection,                        // pushConstants
	};

	std::vector<vk::CommandBuffer> cmdBuffers = {};
	for (auto& pipeline : m_Pipelines)
	{
		cmdBuffers.push_back(pipeline->RecordCommandBuffer(commandBufferStartInfo));
	}

	// Submit commands (and reset the fence)
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo {
		1u,                                       // waitSemaphoreCount
		&m_AquireImageSemaphores[m_CurrentFrame], // pWaitSemaphores
		&waitStage,                               // pWaitDstStageMask
		static_cast<uint32_t>(cmdBuffers.size()), // commandBufferCount
		cmdBuffers.data(),                        // pCommandBuffers
		1u,                                       // signalSemaphoreCount
		&m_RenderSemaphores[m_CurrentFrame],      // pSignalSemaphores
	};

	VKC(m_LogicalDevice.resetFences(1u, &m_FrameFences[m_CurrentFrame]));
	VKC(m_QueueInfo.graphicsQueue.submit(1u, &submitInfo, m_FrameFences[m_CurrentFrame]));

	// Present the image
	vk::PresentInfoKHR presentInfo {
		1u,                                  // waitSemaphoreCount
		&m_RenderSemaphores[m_CurrentFrame], // pWaitSemaphores
		1u,                                  // swapchainCount
		&m_Swapchain,                        // pSwapchains
		&imageIndex,                         // pImageIndices
		nullptr                              // pResults
	};

	try
	{
		result = m_QueueInfo.presentQueue.presentKHR(presentInfo);
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
		{
			m_LogicalDevice.waitIdle();
			m_SwapchainInvalidated = true;
			return;
		}
	}
	catch (vk::OutOfDateKHRError err) // OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	{
		m_LogicalDevice.waitIdle();
		m_SwapchainInvalidated = true;
		return;
	}

	// Increment frame index
	m_CurrentFrame = (m_CurrentFrame + 1u) % m_MaxFramesInFlight;
}

Renderer::~Renderer()
{
	m_LogicalDevice.waitIdle();

	for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
	{
		m_LogicalDevice.destroySemaphore(m_AquireImageSemaphores[i], nullptr);
		m_LogicalDevice.destroySemaphore(m_RenderSemaphores[i], nullptr);
		m_LogicalDevice.destroyFence(m_FrameFences[i], nullptr);
	}

	m_LogicalDevice.destroySwapchainKHR(m_Swapchain, nullptr);
	for (uint32_t i = 0; i < m_Images.size(); i++)
	{
		m_LogicalDevice.destroyImageView(m_ImageViews[i], nullptr);
		m_LogicalDevice.destroyFramebuffer(m_Framebuffers[i], nullptr);
	}

	m_LogicalDevice.destroyImageView(m_DepthImageView, nullptr);
	m_LogicalDevice.destroyImageView(m_ColorImageView, nullptr);
	m_LogicalDevice.destroyImage(m_DepthImage, nullptr);
	m_LogicalDevice.destroyImage(m_ColorImage, nullptr);
	m_LogicalDevice.freeMemory(m_DepthImageMemory, nullptr);
	m_LogicalDevice.freeMemory(m_ColorImageMemory, nullptr);

	m_LogicalDevice.destroyRenderPass(m_RenderPass, nullptr);
	m_LogicalDevice.destroyCommandPool(m_CommandPool, nullptr);
	m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool, nullptr);

	m_LogicalDevice.destroyDescriptorSetLayout(m_DescriptorSetLayout, nullptr);
}

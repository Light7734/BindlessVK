#include "Graphics/Renderer.hpp"

#include "Core/Window.hpp"
#include "Graphics/Types.hpp"
#include "Utils/Timer.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

Renderer::Renderer(const RendererCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_SurfaceInfo(createInfo.surfaceInfo), m_QueueInfo(createInfo.queueInfo), m_SampleCount(createInfo.sampleCount), m_Allocator(createInfo.allocator)
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

			m_RenderSemaphores[i] = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);

			m_FrameFences[i] = m_LogicalDevice.createFence(fenceCreateInfo, nullptr);
		}

		m_UploadContext.fence = m_LogicalDevice.createFence({}, nullptr);
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

		vma::AllocationCreateInfo imageAllocInfo({},
		                                         vma::MemoryUsage::eGpuOnly,
		                                         vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_ColorImage = m_Allocator.createImage(imageCreateInfo, imageAllocInfo);

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

		vma::AllocationCreateInfo imageAllocInfo({},
		                                         vma::MemoryUsage::eGpuOnly,
		                                         vk::MemoryPropertyFlagBits::eDeviceLocal);

		m_DepthImage = m_Allocator.createImage(imageCreateInfo, imageAllocInfo);

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
		m_CommandPool           = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
		m_UploadContext.cmdPool = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);

		vk::CommandBufferAllocateInfo cmdAllocInfo {
			m_UploadContext.cmdPool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};
		m_UploadContext.cmdBuffer = m_LogicalDevice.allocateCommandBuffers(cmdAllocInfo)[0];

		vk::CommandBufferAllocateInfo commandBufferAllocInfo {
			m_CommandPool,                    // commandPool
			vk::CommandBufferLevel::ePrimary, // level
			m_MaxFramesInFlight,              // commandBufferCount
		};
		m_RenderPassCmdBuffer = m_LogicalDevice.allocateCommandBuffers(commandBufferAllocInfo);

		vk::CommandBufferAllocateInfo imguiCommandBufferAllocInfo {
			m_CommandPool,                      // commandPool
			vk::CommandBufferLevel::eSecondary, // level
			m_MaxFramesInFlight,                // commandBufferCount
		};
		m_ImguiCmdBuffers = m_LogicalDevice.allocateCommandBuffers(imguiCommandBufferAllocInfo);
	}

	// Create the texture
	{
		TextureCreateInfo textureCreateInfo {
			m_LogicalDevice,                                                 // logicalDevice
			createInfo.physicalDevice,                                       // physicalDevice
			m_QueueInfo.graphicsQueue,                                       // graphicsQueue
			m_Allocator,                                                     // allocator
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
			.allocator         = m_Allocator,
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


	/////////////////////////////////////////////////////////////////////////////////
	/// Initialize Dear ImGui
	{
		vk::DescriptorPoolSize poolSizes[] = {
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1000ull,
			std::size(poolSizes),
			poolSizes
		};

		m_ImguiPool = m_LogicalDevice.createDescriptorPool(descriptorPoolCreateInfo, nullptr);

		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(createInfo.window->GetGlfwHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo {
			.Instance       = createInfo.instance,
			.PhysicalDevice = createInfo.physicalDevice,
			.Device         = m_LogicalDevice,
			.Queue          = m_QueueInfo.graphicsQueue,
			.DescriptorPool = m_ImguiPool,
			.MinImageCount  = m_MaxFramesInFlight,
			.ImageCount     = m_MaxFramesInFlight,
			.MSAASamples    = static_cast<VkSampleCountFlagBits>(m_SampleCount),
		};


		vk::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		vkGetInstanceProcAddr(NULL, "");

		static struct Payload
		{
			PFN_vkGetInstanceProcAddr vkGetProcAddr;
			vk::Instance instance;
		} payload = { createInfo.procAddr, createInfo.instance };

		ASSERT(ImGui_ImplVulkan_LoadFunctions([](const char* func, void* data) {
			       Payload pload = *(Payload*)data;
			       return payload.vkGetProcAddr(payload.instance, func);
		       },
		                                      (void*)&payload),
		       "ImGui failed to load vulkan functions");

		ImGui_ImplVulkan_Init(&initInfo, m_RenderPass);


		ImmediateSubmit([](vk::CommandBuffer cmd) {
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void Renderer::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
}

void Renderer::Draw()
{
	LOG(critical, "Not implemented!");
}

void Renderer::EndFrame()
{
	if (m_SwapchainInvalidated)
		return;

	static float fov     = 45.0f;
	static glm::vec3 eye = { 4.0f, 4.0f, 2.0f };

	ImGui::SliderFloat("Field of view", &fov, 0.0f, 90.0f);
	ImGui::SliderFloat3("Camera Position", &eye.x, 0.5f, 8.5f);

	ImGui::Render();

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
	m_ViewProjection.view       = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_ViewProjection.projection = glm::perspective(glm::radians(fov), m_SurfaceInfo.capabilities.currentExtent.width / (float)m_SurfaceInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

	// Record commands
	CommandBufferStartInfo commandBufferStartInfo {
		&m_DescriptorSets[m_CurrentFrame],        // descriptorSet
		m_Framebuffers[imageIndex],               // framebuffer
		m_SurfaceInfo.capabilities.currentExtent, // extent
		m_CurrentFrame,                           // frameIndex
		&m_ViewProjection,                        // pushConstants
	};

	vk::CommandBufferBeginInfo beginInfo { {}, nullptr };

	std::array<vk::ClearValue, 2> clearValues {
		vk::ClearValue { vk::ClearColorValue { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } } },
		vk::ClearValue { vk::ClearDepthStencilValue { 1.0f, 0 } },
	};

	vk::RenderPassBeginInfo renderpassBeginInfo {
		m_RenderPass,               // renderPass
		m_Framebuffers[imageIndex], // framebuffer

		/* renderArea */
		vk::Rect2D {
		    { 0, 0 },                                 // offset
		    m_SurfaceInfo.capabilities.currentExtent, // extent
		},
		static_cast<uint32_t>(clearValues.size()), // clearValueCount
		clearValues.data(),                        // pClearValues
	};

	m_RenderPassCmdBuffer[m_CurrentFrame].begin(beginInfo);
	m_RenderPassCmdBuffer[m_CurrentFrame].beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

	std::vector<vk::CommandBuffer> cmdBuffers = {};
	for (auto& pipeline : m_Pipelines)
	{
		cmdBuffers.push_back(pipeline->RecordCommandBuffer(commandBufferStartInfo));
	}


	vk::CommandBufferInheritanceInfo inheritanceInfo {
		m_RenderPass,
		0u,
		m_Framebuffers[imageIndex],
		{},
		{},
		{}
	};
	vk::CommandBufferBeginInfo imguiCmdBeginInfo {
		vk::CommandBufferUsageFlagBits::eRenderPassContinue, // flags
		&inheritanceInfo                                     // pInheritanceInfo
	};

	m_ImguiCmdBuffers[m_CurrentFrame].begin(imguiCmdBeginInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_ImguiCmdBuffers[m_CurrentFrame]);
	m_ImguiCmdBuffers[m_CurrentFrame].end();

	cmdBuffers.push_back(m_ImguiCmdBuffers[m_CurrentFrame]);

	m_RenderPassCmdBuffer[m_CurrentFrame].executeCommands(cmdBuffers);

	m_RenderPassCmdBuffer[m_CurrentFrame].endRenderPass();
	m_RenderPassCmdBuffer[m_CurrentFrame].end();

	//
	// Submit commands (and reset the fence)
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo {
		1u,                                       // waitSemaphoreCount
		&m_AquireImageSemaphores[m_CurrentFrame], // pWaitSemaphores
		&waitStage,                               // pWaitDstStageMask
		1ull,                                     // commandBufferCount
		&m_RenderPassCmdBuffer[m_CurrentFrame],   // pCommandBuffers
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
		if (result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
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

void Renderer::ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function)
{
	vk::CommandBuffer cmd = m_UploadContext.cmdBuffer;

	vk::CommandBufferBeginInfo beginInfo {
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};

	cmd.begin(beginInfo);

	function(cmd);

	cmd.end();

	vk::SubmitInfo submitInfo {
		0u,
		{},
		{},
		1u,
		&cmd,
		0u,
		{},
		{}
	};

	m_QueueInfo.graphicsQueue.submit(submitInfo, m_UploadContext.fence);
	VKC(m_LogicalDevice.waitForFences(m_UploadContext.fence, true, UINT_MAX));
	m_LogicalDevice.resetFences(m_UploadContext.fence);

	m_LogicalDevice.resetCommandPool(m_CommandPool, {});
}

Renderer::~Renderer()
{
	m_LogicalDevice.waitIdle();

	m_LogicalDevice.destroyCommandPool(m_UploadContext.cmdPool);
	m_LogicalDevice.destroyFence(m_UploadContext.fence);

	m_LogicalDevice.destroyDescriptorPool(m_ImguiPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool, nullptr);
	m_LogicalDevice.destroyCommandPool(m_CommandPool, nullptr);

	for (uint32_t i = 0; i < m_Framebuffers.size(); i++)
	{
		m_LogicalDevice.destroyFramebuffer(m_Framebuffers[i], nullptr);
	}

	m_LogicalDevice.destroyRenderPass(m_RenderPass, nullptr);
	m_LogicalDevice.destroyImageView(m_DepthImageView, nullptr);
	m_Allocator.destroyImage(m_DepthImage, m_DepthImage);
	m_LogicalDevice.destroyImageView(m_ColorImageView, nullptr);
	m_Allocator.destroyImage(m_ColorImage, m_ColorImage);

	for (uint32_t i = 0; i < m_ImageViews.size(); i++)
	{
		m_LogicalDevice.destroyImageView(m_ImageViews[i], nullptr);
	}

	m_LogicalDevice.destroySwapchainKHR(m_Swapchain, nullptr);

	for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
	{
		m_LogicalDevice.destroyFence(m_FrameFences[i], nullptr);
		m_LogicalDevice.destroySemaphore(m_RenderSemaphores[i], nullptr);
		m_LogicalDevice.destroySemaphore(m_AquireImageSemaphores[i], nullptr);
	}
}

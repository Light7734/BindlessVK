#include "Graphics/Renderer.hpp"

#include "Core/Window.hpp"
#include "Graphics/Types.hpp"
#include "Utils/CVar.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

AutoCVar ACV_CameraFOV(CVarType::Float, "CameraFOV", "Camera's field of view",
                       45.0f, 45.0f);

Renderer::Renderer(const RendererCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.deviceContext.logicalDevice)
    , m_SurfaceInfo(createInfo.deviceContext.surfaceInfo)
    , m_QueueInfo(createInfo.deviceContext.queueInfo)
    , m_SampleCount(createInfo.deviceContext.maxSupportedSampleCount)
    , m_Allocator(createInfo.deviceContext.allocator)
{
	LOG(err, "FUCKKK!");
	/////////////////////////////////////////////////////////////////////////////////
	/// Create sync objects
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::SemaphoreCreateInfo semaphoreCreateInfo {};

			vk::FenceCreateInfo fenceCreateInfo {
				vk::FenceCreateFlagBits::eSignaled, // flags
			};

			m_Frames[i].renderFence =
			    m_LogicalDevice.createFence(fenceCreateInfo, nullptr);
			m_Frames[i].renderSemaphore =
			    m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
			m_Frames[i].presentSemaphore =
			    m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
		}

		m_UploadContext.fence = m_LogicalDevice.createFence({}, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the swap chain.
	{
		// Select extent

		// Select image count ; one more than minImageCount, if minImageCount +1 is
		// not higher than maxImageCount (maxImageCount of 0 means no limit)
		uint32_t imageCount =
		    m_SurfaceInfo.capabilities.maxImageCount > 0 &&
		            m_SurfaceInfo.capabilities.minImageCount + 1 >
		                m_SurfaceInfo.capabilities.maxImageCount ?
		        m_SurfaceInfo.capabilities.maxImageCount :    // TRUE
		        m_SurfaceInfo.capabilities.minImageCount + 1; // FALSE

		// Create swapchain
		bool sameQueueIndex =
		    m_QueueInfo.graphicsQueueIndex == m_QueueInfo.presentQueueIndex;
		vk::SwapchainCreateInfoKHR swapchainCreateInfo {
			{},                                       // flags
			m_SurfaceInfo.surface,                    // surface
			imageCount,                               // minImageCount
			m_SurfaceInfo.format.format,              // imageFormat
			m_SurfaceInfo.format.colorSpace,          // imageColorSpace
			m_SurfaceInfo.capabilities.currentExtent, // imageExtent
			1u,                                       // imageArrayLayers
			vk::ImageUsageFlagBits::
			    eColorAttachment,                                                        // Write directly to the image (we would use a
			                                                                             // value like VK_IMAGE_USAGE_TRANSFER_DST_BIT if
			                                                                             // we wanted to do post-processing) // imageUsage
			sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
			sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
			sameQueueIndex ? nullptr : &m_QueueInfo.graphicsQueueIndex,                  // pQueueFamilyIndices
			m_SurfaceInfo.capabilities.currentTransform,                                 // preTransform
			vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // No alpha-blending between
			                                                                             // multiple windows //
			                                                                             // compositeAlpha
			m_SurfaceInfo.presentMode,                                                   // presentMode
			VK_TRUE,                                                                     // Don't render the obsecured pixels // clipped
			VK_NULL_HANDLE,                                                              // oldSwapchain
		};

		m_Swapchain =
		    m_LogicalDevice.createSwapchainKHR(swapchainCreateInfo, nullptr);
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
				    vk::ImageAspectFlagBits::eColor, // Image will be used as color
				                                     // target // aspectMask
				    0,                               // No mipmaipping // baseMipLevel
				    1,                               // No levels // levelCount
				    0,                               // No nothin... // baseArrayLayer
				    1,                               // layerCount
				},
			};

			m_ImageViews[i] =
			    m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
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
			1u,                        // mipLevels
			1u,                        // arrayLayers
			m_SampleCount,             // samples
			vk::ImageTiling::eOptimal, // tiling
			vk::ImageUsageFlagBits::eTransientAttachment |
			    vk::ImageUsageFlagBits::eColorAttachment, // usage
			vk::SharingMode::eExclusive,                  // sharingMode
			0u,                                           // queueFamilyIndexCount
			nullptr,                                      // pQueueFamilyIndices
			vk::ImageLayout::eUndefined,                  // initialLayout
		};

		vma::AllocationCreateInfo imageAllocInfo(
		    {}, vma::MemoryUsage::eGpuOnly,
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

		m_ColorImageView =
		    m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create depth buffer
	{
		// Find depth format
		m_DepthFormat            = vk::Format::eUndefined;
		bool hasStencilComponent = false;
		vk::FormatProperties formatProperties;
		for (vk::Format format :
		     { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
		       vk::Format::eD24UnormS8Uint })
		{
			formatProperties =
			    createInfo.deviceContext.physicalDevice.getFormatProperties(format);
			if ((formatProperties.optimalTilingFeatures &
			     vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
			    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				m_DepthFormat       = format;
				hasStencilComponent = format == vk::Format::eD32SfloatS8Uint ||
				                      format == vk::Format::eD24UnormS8Uint;
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

		vma::AllocationCreateInfo imageAllocInfo(
		    {}, vma::MemoryUsage::eGpuOnly,
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

		m_DepthImageView =
		    m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Specify the attachments and subpasses and create the forward pass
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
			VK_SUBPASS_EXTERNAL, // srcSubpass
			0u,                  // dstSubpass
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			    vk::PipelineStageFlagBits::eEarlyFragmentTests, // srcStageMask
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			    vk::PipelineStageFlagBits::eEarlyFragmentTests, // dstStageMask
			{},                                                 // srcAccessMask
			vk::AccessFlagBits::eColorAttachmentWrite |
			    vk::AccessFlagBits::eDepthStencilAttachmentWrite, // dstAccessMask
			{},                                                   // dependencytFlags
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

		m_ForwardPass.renderpass =
		    m_LogicalDevice.createRenderPass(renderPassCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the framebuffers
	{
		m_ForwardPass.framebuffers.resize(m_Images.size());
		for (uint32_t i = 0; i < m_ForwardPass.framebuffers.size(); i++)
		{
			std::vector<vk::ImageView> imageViews = {
				m_ColorImageView, m_DepthImageView, m_ImageViews[i]
			};
			vk::FramebufferCreateInfo framebufferCreateInfo {
				{},                                              // flags
				m_ForwardPass.renderpass,                        // renderPass
				static_cast<uint32_t>(imageViews.size()),        // attachmentCount
				imageViews.data(),                               // pAttachments
				m_SurfaceInfo.capabilities.currentExtent.width,  // width
				m_SurfaceInfo.capabilities.currentExtent.height, // height
				1u,                                              // layers
			};

			m_ForwardPass.framebuffers[i] =
			    m_LogicalDevice.createFramebuffer(framebufferCreateInfo, nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the command pool
	{
		// renderer
		vk::CommandPoolCreateInfo commandPoolCreateInfo {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
			m_QueueInfo.graphicsQueueIndex,                     // queueFamilyIndex
		};

		m_CommandPool =
		    m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
		m_ForwardPass.cmdPool =
		    m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
		m_UploadContext.cmdPool =
		    m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);

		vk::CommandBufferAllocateInfo uploadContextCmdBufferAllocInfo {
			m_UploadContext.cmdPool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};
		m_UploadContext.cmdBuffer = m_LogicalDevice.allocateCommandBuffers(
		    uploadContextCmdBufferAllocInfo)[0];

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::CommandBufferAllocateInfo primaryCmdBufferAllocInfo {
				m_ForwardPass.cmdPool,            // commandPool
				vk::CommandBufferLevel::ePrimary, // level
				1ull,                             // commandBufferCount
			};

			vk::CommandBufferAllocateInfo secondaryCmdBuffersAllocInfo {
				m_CommandPool,                      // commandPool
				vk::CommandBufferLevel::eSecondary, // level
				MAX_FRAMES_IN_FLIGHT,               // commandBufferCount
			};

			m_ForwardPass.cmdBuffers[i].primary =
			    m_LogicalDevice.allocateCommandBuffers(primaryCmdBufferAllocInfo)[0];
			m_ForwardPass.cmdBuffers[i].secondaries =
			    m_LogicalDevice.allocateCommandBuffers(secondaryCmdBuffersAllocInfo);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create texture - @todo wtf is this doing here?
	{
		TextureCreateInfo textureCreateInfo {
			m_LogicalDevice,                         // logicalDevice
			createInfo.deviceContext.physicalDevice, // physicalDevice
			m_QueueInfo.graphicsQueue,               // graphicsQueue
			m_Allocator,                             // allocator
			m_CommandPool,                           // commandPool
			"Assets/viking_room.png",                // imagePath
			VK_TRUE,                                 // anisotropyEnabled
			createInfo.deviceContext.physicalDeviceProperties.limits
			    .maxSamplerAnisotropy, // maxAnisotropy
		};

		m_TempTexture = std::make_shared<Texture>(textureCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create descriptor pool
	{
		std::vector<vk::DescriptorPoolSize> poolSizes = {
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
		// descriptorPoolSizes.push_back(VkDescriptorPoolSize {});

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {
			{},                                      // flags
			100,                                     // maxSets
			static_cast<uint32_t>(poolSizes.size()), // poolSizeCount
			poolSizes.data(),                        // pPoolSizes
		};

		m_DescriptorPool =
		    m_LogicalDevice.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create camera data buffer
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			BufferCreateInfo createInfo {

				m_LogicalDevice,                         // logicalDevice
				createInfo.physicalDevice,               // physicalDevice
				m_Allocator,                             // vmaallocator
				m_CommandPool,                           // commandPool
				m_QueueInfo.graphicsQueue,               // graphicsQueue
				vk::BufferUsageFlagBits::eUniformBuffer, // usage
				sizeof(glm::mat4) * 2ul,                 // size
				{}                                       // inital data
			};

			m_Frames[i].cameraData.buffer = std::make_unique<Buffer>(createInfo);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create frame descriptor sets
	{
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			// [0] UniformBuffer - View Projection
			vk::DescriptorSetLayoutBinding {
			    0ul,
			    vk::DescriptorType::eUniformBuffer,
			    1ul,
			    vk::ShaderStageFlagBits::eVertex,
			    nullptr,
			},
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
			{},
			static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
			descriptorSetLayoutBindings.data(),
		};
		LOG(warn, "Creating frame descriptor set layout");
		m_FramesDescriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(
		    descriptorSetLayoutCreateInfo, nullptr);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1ul,
				&m_FramesDescriptorSetLayout,
			};
			m_Frames[i].descriptorSet =
			    m_LogicalDevice.allocateDescriptorSets(allocInfo)[0];
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create forward pass descriptor sets
	{
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			// [0] StorageBuffer - Object Data
			vk::DescriptorSetLayoutBinding {
			    0ul,                                // binding
			    vk::DescriptorType::eStorageBuffer, // descriptorType
			    1ul,                                // descriptorCount
			    vk::ShaderStageFlagBits::eVertex,   // stageFlags
			    nullptr,                            // pImmutableSamplers
			},

			// [1] Sampler - Texture
			vk::DescriptorSetLayoutBinding {
			    1u,                                        // binding
			    vk::DescriptorType::eCombinedImageSampler, // descriptorType
			    1u,                                        // descriptorCount
			    vk::ShaderStageFlagBits::eFragment,        // stageFlags
			    nullptr,                                   // pImmutableSamplers
			},
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
			{}, // flags
			static_cast<uint32_t>(
			    descriptorSetLayoutBindings.size()), // bindingCount
			descriptorSetLayoutBindings.data(),      // pBindings
		};

		LOG(warn, "Creating forward pass descriptor set layout");
		m_ForwardPass.descriptorSetLayout =
		    m_LogicalDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo,
		                                              nullptr);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1ul,
				&m_ForwardPass.descriptorSetLayout,
			};

			m_ForwardPass.descriptorSets[i] =
			    m_LogicalDevice.allocateDescriptorSets(allocInfo)[0];
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create frame pipeline layout
	{
		std::array<vk::DescriptorSetLayout, 1> setLayouts = {
			m_FramesDescriptorSetLayout,
		};
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
			{},                // flags
			setLayouts.size(), // setLayoutCount
			setLayouts.data(), //// pSetLayouts
		};

		m_FramePipelineLayout =
		    m_LogicalDevice.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create forward pass pipeline layout
	{
		std::array<vk::DescriptorSetLayout, 2> setLayouts = {
			m_FramesDescriptorSetLayout,
			m_ForwardPass.descriptorSetLayout,
		};
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
			{},                // flags
			setLayouts.size(), // setLayoutCount
			setLayouts.data(), //// pSetLayouts
		};

		m_ForwardPass.pipelineLayout =
		    m_LogicalDevice.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create forward pass storage buffer
	{
		BufferCreateInfo createInfo {
			.logicalDevice  = m_LogicalDevice,
			.physicalDevice = createInfo.physicalDevice,
			.allocator      = m_Allocator,
			.commandPool    = m_CommandPool,
			.graphicsQueue  = m_QueueInfo.graphicsQueue,
			.usage          = vk::BufferUsageFlagBits::eStorageBuffer,
			.size           = sizeof(glm::mat4) * 100,
			.initialData    = {},
		};

		m_ForwardPass.storageBuffer = std::make_unique<Buffer>(createInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create pipelines @todo automate this??
	{
		// Update model view projection uniform
		RenderableCreateInfo renderableCreateInfo {
			"Assets/viking_room.obj", // modelPath
			{ m_TempTexture }         // #TODO // textures
		};

		std::vector<vk::DescriptorSetLayout> setLayouts = {
			m_FramesDescriptorSetLayout,
			m_ForwardPass.descriptorSetLayout,
		};

		// #TODO:
		LOG(warn, "Creating pipeline...");
		PipelineCreateInfo pipelineCreateInfo {
			.logicalDevice        = m_LogicalDevice,
			.physicalDevice       = createInfo.deviceContext.physicalDevice,
			.allocator            = m_Allocator,
			.maxFramesInFlight    = MAX_FRAMES_IN_FLIGHT,
			.queueInfo            = m_QueueInfo,
			.viewportExtent       = m_SurfaceInfo.capabilities.currentExtent,
			.commandPool          = m_CommandPool,
			.imageCount           = static_cast<uint32_t>(m_Images.size()),
			.sampleCount          = createInfo.deviceContext.maxSupportedSampleCount,
			.renderPass           = m_ForwardPass.renderpass,
			.descriptorSetLayouts = setLayouts,
			.descriptorPool       = m_DescriptorPool,

			// Shader
			.vertexShaderPath = "Shaders/vertex.glsl",
			.pixelShaderPath  = "Shaders/pixel.glsl",

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
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(createInfo.window->GetGlfwHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo {
			.Instance       = createInfo.deviceContext.instance,
			.PhysicalDevice = createInfo.deviceContext.physicalDevice,
			.Device         = m_LogicalDevice,
			.Queue          = m_QueueInfo.graphicsQueue,
			.DescriptorPool = m_DescriptorPool,
			.MinImageCount  = MAX_FRAMES_IN_FLIGHT,
			.ImageCount     = MAX_FRAMES_IN_FLIGHT,
			.MSAASamples    = static_cast<VkSampleCountFlagBits>(m_SampleCount),
		};

		std::pair userData =
		    std::make_pair(createInfo.deviceContext.vkGetInstanceProcAddr,
		                   createInfo.deviceContext.instance);

		ASSERT(ImGui_ImplVulkan_LoadFunctions(
		           [](const char* func, void* data) {
			           auto [vkGetProcAddr, instance] = *(
			               std::pair<PFN_vkGetInstanceProcAddr, vk::Instance>*)data;
			           return vkGetProcAddr(instance, func);
		           },
		           (void*)&userData),
		       "ImGui failed to load vulkan functions");

		ImGui_ImplVulkan_Init(&initInfo, m_ForwardPass.renderpass);

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
	CVar::DrawImguiEditor();
}

void Renderer::Draw()
{
	LOG(critical, "Not implemented!");
}

void Renderer::EndFrame()
{
	if (m_SwapchainInvalidated)
		return;

	ImGui::Render();

	/////////////////////////////////////////////////////////////////////////////////
	/// Get frame & image data
	const FrameData& frame = m_Frames[m_CurrentFrame];

	// Wait for the frame fence
	VKC(m_LogicalDevice.waitForFences(1u, &frame.renderFence, VK_TRUE,
	                                  UINT64_MAX));

	// Acquire an image
	uint32_t imageIndex;
	vk::Result result = m_LogicalDevice.acquireNextImageKHR(
	    m_Swapchain, UINT64_MAX, frame.renderSemaphore, VK_NULL_HANDLE,
	    &imageIndex);
	if (result == vk::Result::eErrorOutOfDateKHR ||
	    result == vk::Result::eSuboptimalKHR || m_SwapchainInvalidated)
	{
		m_LogicalDevice.waitIdle();
		m_SwapchainInvalidated = true;
		return;
	}
	else
	{
		ASSERT(result == vk::Result::eSuccess,
		       "VkAcquireNextImage failed without returning "
		       "VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR");
	}

	////////////////////////////////////////////////////////////////
	/// Update frame descriptor set
	{
		glm::mat4* map = (glm::mat4*)frame.cameraData.buffer->Map();
		map[0]         = glm::perspective(
		            glm::radians((float)CVar::Get("CameraFOV")),
		            m_SurfaceInfo.capabilities.currentExtent.width /
		                (float)m_SurfaceInfo.capabilities.currentExtent.height,
		            0.1f, 10.0f);
		map[1] = glm::lookAt({ 4.0f, 4.0f, 2.0f }, glm::vec3(0.0f, 0.0f, 0.0f),
		                     glm::vec3(0.0f, 0.0f, -1.0f));
		frame.cameraData.buffer->Unmap();

		vk::DescriptorBufferInfo viewProjectionBufferInfo {
			*frame.cameraData.buffer->GetBuffer(),
			0ul,
			VK_WHOLE_SIZE,
		};

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
			vk::WriteDescriptorSet {
			    m_Frames[m_CurrentFrame].descriptorSet,
			    0ul,
			    0ul,
			    1ul,
			    vk::DescriptorType::eUniformBuffer,
			    nullptr,
			    &viewProjectionBufferInfo,
			    nullptr,
			},
		};

		m_LogicalDevice.updateDescriptorSets(
		    static_cast<uint32_t>(writeDescriptorSets.size()),
		    writeDescriptorSets.data(), 0ul, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Forward Render Pass
	{
		const std::array<vk::ClearValue, 2> clearValues {
			vk::ClearValue {
			    vk::ClearColorValue { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } } },
			vk::ClearValue { vk::ClearDepthStencilValue { 1.0f, 0 } },
		};

		const vk::Framebuffer framebuffer = m_ForwardPass.framebuffers[imageIndex];

		const vk::CommandBufferInheritanceInfo inheritanceInfo {
			m_ForwardPass.renderpass, // renderpass
			0u,                       // subpass
			framebuffer,              // framebuffer
		};

		const vk::CommandBufferBeginInfo primaryCmdBufferBeginInfo {
			{},     // flags
			nullptr // pInheritanceInfo
		};

		const vk::CommandBufferBeginInfo secondaryCmdBufferBeginInfo {
			vk::CommandBufferUsageFlagBits::eRenderPassContinue, // flags
			&inheritanceInfo                                     // pInheritanceInfo
		};

		const vk::RenderPassBeginInfo renderpassBeginInfo {
			m_ForwardPass.renderpass, // renderPass
			framebuffer,              // framebuffer

			/* renderArea */
			vk::Rect2D {
			    { 0, 0 },                                 // offset
			    m_SurfaceInfo.capabilities.currentExtent, // extent
			},
			static_cast<uint32_t>(clearValues.size()), // clearValueCount
			clearValues.data(),                        // pClearValues
		};

		// Record commnads @todo multi-thread secondary commands
		const auto primaryCmd = m_ForwardPass.cmdBuffers[m_CurrentFrame].primary;
		const auto secondaryCmds =
		    m_ForwardPass.cmdBuffers[m_CurrentFrame].secondaries[m_CurrentFrame];

		primaryCmd.reset();
		primaryCmd.begin(primaryCmdBufferBeginInfo);
		primaryCmd.beginRenderPass(renderpassBeginInfo,
		                           vk::SubpassContents::eSecondaryCommandBuffers);

		// Update descriptor sets
		const vk::DescriptorSet descriptorSet =
		    m_ForwardPass.descriptorSets[m_CurrentFrame];

		vk::DescriptorImageInfo descriptorImageInfo {
			m_TempTexture->GetSampler(),             // sampler
			m_TempTexture->GetImageView(),           // imageView
			vk::ImageLayout::eShaderReadOnlyOptimal, // imageLayout
		};

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets {
			vk::WriteDescriptorSet {
			    descriptorSet,
			    1u,
			    0u,
			    1u,
			    vk::DescriptorType::eCombinedImageSampler,
			    &descriptorImageInfo,
			    nullptr,
			    nullptr,
			},
		};
		secondaryCmds.reset();

		// #TODO WTF??
		secondaryCmds.begin(secondaryCmdBufferBeginInfo);
		secondaryCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 m_FramePipelineLayout, 0ul, 1ul,
		                                 &frame.descriptorSet, 0ul, nullptr);
		secondaryCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 m_ForwardPass.pipelineLayout, 1ul, 1ul,
		                                 &descriptorSet, 0ul, nullptr);
		m_LogicalDevice.updateDescriptorSets(
		    static_cast<uint32_t>(writeDescriptorSets.size()),
		    writeDescriptorSets.data(), 0u, nullptr);

		// Record pipeline commands
		for (auto& pipeline : m_Pipelines)
		{
			pipeline->RecordCommandBuffer(secondaryCmds);
		}
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), secondaryCmds);
		secondaryCmds.end();

		primaryCmd.executeCommands(1ul, &secondaryCmds);
		primaryCmd.endRenderPass();
		primaryCmd.end();
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// UserInterface Render Pass
	{}

	/////////////////////////////////////////////////////////////////////////////////
	/// Submit commands (and reset the fence)
	{
		std::vector<vk::CommandBuffer> primaryCmdBuffers {
			m_ForwardPass.cmdBuffers[m_CurrentFrame].primary,
		};

		vk::PipelineStageFlags waitStage =
		    vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submitInfo {
			1u,                                              // waitSemaphoreCount
			&frame.renderSemaphore,                          // pWaitSemaphores
			&waitStage,                                      // pWaitDstStageMask
			static_cast<uint32_t>(primaryCmdBuffers.size()), // commandBufferCount
			primaryCmdBuffers.data(),                        // pCommandBuffers
			1u,                                              // signalSemaphoreCount
			&frame.presentSemaphore,                         // pSignalSemaphores
		};

		VKC(m_LogicalDevice.resetFences(1u, &frame.renderFence));
		VKC(m_QueueInfo.graphicsQueue.submit(1u, &submitInfo, frame.renderFence));
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Present the frame
	{
		vk::PresentInfoKHR presentInfo {
			1u,                      // waitSemaphoreCount
			&frame.presentSemaphore, // pWaitSemaphores
			1u,                      // swapchainCount
			&m_Swapchain,            // pSwapchains
			&imageIndex,             // pImageIndices
			nullptr                  // pResults
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
		catch (
		    vk::OutOfDateKHRError err) // OutOfDateKHR is not considered a success
		                               // value and throws an error (presentKHR)
		{
			m_LogicalDevice.waitIdle();
			m_SwapchainInvalidated = true;
			return;
		}
	}

	// Increment frame index
	m_CurrentFrame = (m_CurrentFrame + 1u) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::ImmediateSubmit(
    std::function<void(vk::CommandBuffer)>&& function)
{
	vk::CommandBuffer cmd = m_UploadContext.cmdBuffer;

	vk::CommandBufferBeginInfo beginInfo {
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};

	cmd.begin(beginInfo);

	function(cmd);

	cmd.end();

	vk::SubmitInfo submitInfo { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };

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

	for (uint32_t i = 0; i < m_ForwardPass.framebuffers.size(); i++)
	{
		m_LogicalDevice.destroyFramebuffer(m_ForwardPass.framebuffers[i], nullptr);
	}

	m_LogicalDevice.destroyRenderPass(m_ForwardPass.renderpass, nullptr);
	m_LogicalDevice.destroyImageView(m_DepthImageView, nullptr);
	m_Allocator.destroyImage(m_DepthImage, m_DepthImage);
	m_LogicalDevice.destroyImageView(m_ColorImageView, nullptr);
	m_Allocator.destroyImage(m_ColorImage, m_ColorImage);

	for (uint32_t i = 0; i < m_ImageViews.size(); i++)
	{
		m_LogicalDevice.destroyImageView(m_ImageViews[i], nullptr);
	}

	m_LogicalDevice.destroySwapchainKHR(m_Swapchain, nullptr);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_LogicalDevice.destroyFence(m_Frames[i].renderFence, nullptr);
		m_LogicalDevice.destroySemaphore(m_Frames[i].renderSemaphore, nullptr);
		m_LogicalDevice.destroySemaphore(m_Frames[i].presentSemaphore, nullptr);
	}
}

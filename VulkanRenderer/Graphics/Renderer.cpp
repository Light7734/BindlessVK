#include "Graphics/Renderer.hpp"

#include "Core/Window.hpp"
#include "Graphics/Types.hpp"
#include "Scene/Scene.hpp"
#include "Utils/CVar.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <imgui.h>

Renderer::Renderer(const RendererCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.deviceContext.logicalDevice)
    , m_Allocator(createInfo.deviceContext.allocator)
    , m_DefaultTexture(createInfo.defaultTexture)
    , m_SkyboxTexture(createInfo.skyboxTexture)

{
	/////////////////////////////////////////////////////////////////////////////////
	/// Create sync objects
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::SemaphoreCreateInfo semaphoreCreateInfo {};

			vk::FenceCreateInfo fenceCreateInfo {
				vk::FenceCreateFlagBits::eSignaled, // flags
			};

			m_Frames[i].renderFence      = m_LogicalDevice.createFence(fenceCreateInfo, nullptr);
			m_Frames[i].renderSemaphore  = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
			m_Frames[i].presentSemaphore = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
		}

		m_UploadContext.fence = m_LogicalDevice.createFence({}, nullptr);
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

		m_DescriptorPool = m_LogicalDevice.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create swapchain
	{
		RecreateSwapchain(createInfo.window, createInfo.deviceContext);
	}
}

void Renderer::RecreateSwapchain(Window* window, DeviceContext deviceContext)
{
	DestroySwapchain();

	m_SurfaceInfo = deviceContext.surfaceInfo;
	m_QueueInfo   = deviceContext.queueInfo;
	m_SampleCount = deviceContext.maxSupportedSampleCount;

	/////////////////////////////////////////////////////////////////////////////////
	// Create the swap chain.
	{
		const uint32_t minImage = m_SurfaceInfo.capabilities.minImageCount;
		const uint32_t maxImage = m_SurfaceInfo.capabilities.maxImageCount;

		uint32_t imageCount = maxImage >= DESIRED_SWAPCHAIN_IMAGES && minImage <= DESIRED_SWAPCHAIN_IMAGES ? DESIRED_SWAPCHAIN_IMAGES :
		                      maxImage == 0ul && minImage <= DESIRED_SWAPCHAIN_IMAGES                      ? DESIRED_SWAPCHAIN_IMAGES :
		                      minImage <= 2ul && maxImage >= 2ul                                           ? 2ul :
		                      minImage == 0ul && maxImage >= 2ul                                           ? 2ul :
		                                                                                                     minImage;
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
			vk::ImageUsageFlagBits::eColorAttachment,                                    // imageUsage -> Write directly to the image (use VK_IMAGE_USAGE_TRANSFER_DST_BIT for post-processing) ??
			sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
			sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
			sameQueueIndex ? nullptr : &m_QueueInfo.graphicsQueueIndex,                  // pQueueFamilyIndices
			m_SurfaceInfo.capabilities.currentTransform,                                 // preTransform
			vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // compositeAlpha -> No alpha-blending between multiple windows
			m_SurfaceInfo.presentMode,                                                   // presentMode
			VK_TRUE,                                                                     // clipped -> Don't render the obsecured pixels
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
				    vk::ImageAspectFlagBits::eColor, // Image will be used as color
				                                     // target // aspectMask
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

		m_ColorImageView = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create depth buffer
	{
		// Create depth image
		vk::ImageCreateInfo imageCreateInfo {
			{},                        // flags
			vk::ImageType::e2D,        // imageType
			deviceContext.depthFormat, // format

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

		vma::AllocationCreateInfo imageAllocInfo({}, vma::MemoryUsage::eGpuOnly, vk::MemoryPropertyFlagBits::eDeviceLocal);

		m_DepthImage = m_Allocator.createImage(imageCreateInfo, imageAllocInfo);

		// Create depth image-view
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                        // flags
			m_DepthImage,              // image
			vk::ImageViewType::e2D,    // viewType
			deviceContext.depthFormat, // format

			/* components */
			vk::ComponentMapping {
			    // Don't swizzle the colors around...
			    vk::ComponentSwizzle::eIdentity, // t
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
	// Create the command pool
	{
		// renderer
		vk::CommandPoolCreateInfo commandPoolCreateInfo {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
			m_QueueInfo.graphicsQueueIndex,                     // queueFamilyIndex
		};

		m_CommandPool           = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
		m_ForwardPass.cmdPool   = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
		m_UploadContext.cmdPool = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);

		vk::CommandBufferAllocateInfo uploadContextCmdBufferAllocInfo {
			m_UploadContext.cmdPool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};
		m_UploadContext.cmdBuffer = m_LogicalDevice.allocateCommandBuffers(uploadContextCmdBufferAllocInfo)[0];

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

			m_ForwardPass.cmdBuffers[i].primary     = m_LogicalDevice.allocateCommandBuffers(primaryCmdBufferAllocInfo)[0];
			m_ForwardPass.cmdBuffers[i].secondaries = m_LogicalDevice.allocateCommandBuffers(secondaryCmdBuffersAllocInfo);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create camera data buffer
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			BufferCreateInfo createInfo {

				m_LogicalDevice,                                       // logicalDevice
				deviceContext.physicalDevice,                          // physicalDevice
				m_Allocator,                                           // vmaallocator
				m_CommandPool,                                         // commandPool
				m_QueueInfo.graphicsQueue,                             // graphicsQueue
				vk::BufferUsageFlagBits::eUniformBuffer,               // usage
				(sizeof(glm::mat4) * 2ul) + (sizeof(glm::vec4) * 2ul), // size
				{}                                                     // inital data
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
		m_FramesDescriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(
		    descriptorSetLayoutCreateInfo, nullptr);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1ul,
				&m_FramesDescriptorSetLayout,
			};
			m_Frames[i].descriptorSet = m_LogicalDevice.allocateDescriptorSets(allocInfo)[0];

			vk::DescriptorBufferInfo viewProjectionBufferInfo {
				*m_Frames[i].cameraData.buffer->GetBuffer(),
				0ul,
				VK_WHOLE_SIZE,
			};

			std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
				vk::WriteDescriptorSet {
				    m_Frames[i].descriptorSet,
				    0ul,
				    0ul,
				    1ul,
				    vk::DescriptorType::eUniformBuffer,
				    nullptr,
				    &viewProjectionBufferInfo,
				    nullptr,
				},
			};

			m_LogicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0ul, nullptr);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create forward pass descriptor sets
	{
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			// [0] Sampler - Texture
			vk::DescriptorSetLayoutBinding {
			    0u,                                        // binding
			    vk::DescriptorType::eCombinedImageSampler, // descriptorType
			    32u,                                       // descriptorCount
			    vk::ShaderStageFlagBits::eFragment,        // stageFlags
			    nullptr,                                   // pImmutableSamplers
			},

			// [1] Sampler - Texture
			vk::DescriptorSetLayoutBinding {
			    1u,                                        // binding
			    vk::DescriptorType::eCombinedImageSampler, // descriptorType
			    8u,                                        // descriptorCount
			    vk::ShaderStageFlagBits::eFragment,        // stageFlags
			    nullptr,                                   // pImmutableSamplers
			},
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
			{},                                                        // flags
			static_cast<uint32_t>(descriptorSetLayoutBindings.size()), // bindingCount
			descriptorSetLayoutBindings.data(),                        // pBindings
		};

		m_ForwardPass.descriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, nullptr);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1ul,
				&m_ForwardPass.descriptorSetLayout,
			};

			m_ForwardPass.descriptorSets[i] = m_LogicalDevice.allocateDescriptorSets(allocInfo)[0];
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

		m_FramePipelineLayout = m_LogicalDevice.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);
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

		m_ForwardPass.pipelineLayout = m_LogicalDevice.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Create forward pass storage buffer
	{
		BufferCreateInfo createInfo {
			.logicalDevice  = m_LogicalDevice,
			.physicalDevice = deviceContext.physicalDevice,
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
	/// Initialize Dear ImGui
	{
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(window->GetGlfwHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo {
			.Instance              = deviceContext.instance,
			.PhysicalDevice        = deviceContext.physicalDevice,
			.Device                = m_LogicalDevice,
			.Queue                 = m_QueueInfo.graphicsQueue,
			.DescriptorPool        = m_DescriptorPool,
			.UseDynamicRendering   = true,
			.ColorAttachmentFormat = static_cast<VkFormat>(m_SurfaceInfo.format.format),
			.MinImageCount         = MAX_FRAMES_IN_FLIGHT,
			.ImageCount            = MAX_FRAMES_IN_FLIGHT,
			.MSAASamples           = static_cast<VkSampleCountFlagBits>(m_SampleCount),
		};

		std::pair userData = std::make_pair(deviceContext.vkGetInstanceProcAddr, deviceContext.instance);

		ASSERT(ImGui_ImplVulkan_LoadFunctions(
		           [](const char* func, void* data) {
			           auto [vkGetProcAddr, instance] = *(std::pair<PFN_vkGetInstanceProcAddr, vk::Instance>*)data;
			           return vkGetProcAddr(instance, func);
		           },
		           (void*)&userData),
		       "ImGui failed to load vulkan functions");

		ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

		ImmediateSubmit([](vk::CommandBuffer cmd) {
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	ASSERT(m_DefaultTexture, "Renderer's default texture is not set");
	ASSERT(m_SkyboxTexture, "Renderer's skybox texture is not set");
	std::vector<vk::WriteDescriptorSet> descriptorWrites;
	for (uint32_t i = 0; i < 32; i++)
	{
		for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
		{
			descriptorWrites.push_back(vk::WriteDescriptorSet {
			    m_ForwardPass.descriptorSets[j],
			    0ul,
			    i,
			    1ul,
			    vk::DescriptorType::eCombinedImageSampler,
			    &m_DefaultTexture->descriptorInfo,
			    nullptr,
			    nullptr,
			});
		}
	}

	for (uint32_t i = 0; i < 8; i++)
	{
		for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
		{
			descriptorWrites.push_back(vk::WriteDescriptorSet {
			    m_ForwardPass.descriptorSets[j],
			    1ul,
			    i,
			    1ul,
			    vk::DescriptorType::eCombinedImageSampler,
			    &m_SkyboxTexture->descriptorInfo,
			    nullptr,
			    nullptr,
			});
		}
	}
	m_LogicalDevice.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0ull, nullptr);

	m_SwapchainInvalidated = false;
	m_LogicalDevice.waitIdle();
}


void Renderer::DestroySwapchain()
{
	if (!m_Swapchain)
	{
		return;
	}

	m_LogicalDevice.waitIdle();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	m_LogicalDevice.resetCommandPool(m_CommandPool);
	m_LogicalDevice.resetCommandPool(m_UploadContext.cmdPool);
	m_LogicalDevice.destroyCommandPool(m_ForwardPass.cmdPool);
	m_LogicalDevice.destroyCommandPool(m_UploadContext.cmdPool);
	m_LogicalDevice.destroyCommandPool(m_CommandPool);

	m_LogicalDevice.resetDescriptorPool(m_DescriptorPool);

	for (const auto& imageView : m_ImageViews)
	{
		m_LogicalDevice.destroyImageView(imageView);
	}

	m_LogicalDevice.destroyImageView(m_DepthImageView);
	m_LogicalDevice.destroyImageView(m_ColorImageView);

	m_Allocator.destroyImage(m_DepthImage, m_DepthImage);
	m_Allocator.destroyImage(m_ColorImage, m_ColorImage);

	m_LogicalDevice.destroyDescriptorSetLayout(m_FramesDescriptorSetLayout);
	m_LogicalDevice.destroyDescriptorSetLayout(m_ForwardPass.descriptorSetLayout);

	m_LogicalDevice.destroyPipelineLayout(m_FramePipelineLayout);
	m_LogicalDevice.destroyPipelineLayout(m_ForwardPass.pipelineLayout);

	m_LogicalDevice.destroySwapchainKHR(m_Swapchain);
};

Renderer::~Renderer()
{
	DestroySwapchain();

	m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool, nullptr);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_LogicalDevice.destroyFence(m_Frames[i].renderFence, nullptr);
		m_LogicalDevice.destroySemaphore(m_Frames[i].renderSemaphore, nullptr);
		m_LogicalDevice.destroySemaphore(m_Frames[i].presentSemaphore, nullptr);
	}

	m_LogicalDevice.destroyFence(m_UploadContext.fence);
}
void Renderer::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
	CVar::DrawImguiEditor();
}

void Renderer::DrawScene(Scene* scene, const Camera& camera)
{
	if (m_SwapchainInvalidated)
		return;


	ImGui::Render();

	/////////////////////////////////////////////////////////////////////////////////
	/// Get frame & image data
	const FrameData& frame = m_Frames[m_CurrentFrame];

	// Wait for the frame fence
	VKC(m_LogicalDevice.waitForFences(1u, &frame.renderFence, VK_TRUE, UINT64_MAX));

	// Acquire an image
	uint32_t imageIndex;
	vk::Result result = m_LogicalDevice.acquireNextImageKHR(m_Swapchain, UINT64_MAX, frame.renderSemaphore, VK_NULL_HANDLE, &imageIndex);
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


	scene->GetRegistry()->group(entt::get<TransformComponent, StaticMeshRendererComponent>).each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
		for (const auto* node : renderComp.model->nodes)
		{
			for (const auto& primitive : node->mesh)
			{
				const auto& model    = renderComp.model;
				const auto& material = model->materialParameters[primitive.materialIndex];

				uint32_t albedoTextureIndex            = material.albedoTextureIndex;
				uint32_t normalTextureIndex            = material.normalTextureIndex;
				uint32_t metallicRoughnessTextureIndex = material.metallicRoughnessTextureIndex;

				Texture* albedoTexture            = model->textures[albedoTextureIndex];
				Texture* normalTexture            = model->textures[normalTextureIndex];
				Texture* metallicRoughnessTexture = model->textures[metallicRoughnessTextureIndex];

				std::vector<vk::WriteDescriptorSet> descriptorWrites = {
					vk::WriteDescriptorSet {
					    m_ForwardPass.descriptorSets[m_CurrentFrame],
					    0ul,
					    albedoTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &albedoTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					},
					vk::WriteDescriptorSet {
					    m_ForwardPass.descriptorSets[m_CurrentFrame],
					    0ul,
					    metallicRoughnessTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &metallicRoughnessTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					},
					vk::WriteDescriptorSet {
					    m_ForwardPass.descriptorSets[m_CurrentFrame],
					    0ul,
					    normalTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &normalTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					}
				};

				m_LogicalDevice.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0ull, nullptr);
			}
		}
	});

	////////////////////////////////////////////////////////////////
	/// Update frame descriptor set
	{
		struct PerFrameData
		{
			glm::mat4 projection;
			glm::mat4 view;
			glm::vec4 lightPos;
			glm::vec4 viewPos;
		};

		PerFrameData* map = (PerFrameData*)frame.cameraData.buffer->Map();
		map->projection   = camera.GetProjection();
		map->view         = camera.GetView();
		map->lightPos     = glm::vec4(2.0f, 2.0f, 1.0f, 1.0f);
		map->viewPos      = camera.GetPosition();
		frame.cameraData.buffer->Unmap();
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Rendering
	{
		vk::RenderingAttachmentInfo colorAttachmentInfo {
			m_ColorImageView,                         // imageView
			vk::ImageLayout::eColorAttachmentOptimal, // imageLayout

			vk::ResolveModeFlagBits::eAverage,        // resolveMode
			m_ImageViews[imageIndex],                 // resolveImageView
			vk::ImageLayout::eColorAttachmentOptimal, // resolveImageLayout

			vk::AttachmentLoadOp::eClear,  // loadOp
			vk::AttachmentStoreOp::eStore, // storeOp

			/* clearValue */
			vk::ClearColorValue {
			    std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f },
			},
		};

		vk::RenderingAttachmentInfo depthAttachmentInfo {
			m_DepthImageView,                                // imageView
			vk::ImageLayout::eDepthStencilAttachmentOptimal, // imageLayout

			{}, // resolveMode
			{}, // resolveImageView
			{}, // resolveImageLayout

			vk::AttachmentLoadOp::eClear,  // loadOp
			vk::AttachmentStoreOp::eStore, // storeOp

			/* clearValue */
			vk::ClearDepthStencilValue {
			    { 1.0, 0 },
			},
		};


		vk::RenderingInfo renderingInfo {
			{}, // flags

			/* renderArea */
			vk::Rect2D {
			    { 0, 0 },                                 // offset
			    m_SurfaceInfo.capabilities.currentExtent, // extent
			},

			1u,                   // layerCount
			{},                   // viewMask
			1u,                   // colorAttachmentCount
			&colorAttachmentInfo, // pColorAttachments
			&depthAttachmentInfo, // pDepthAttachment
			{},                   // pStencilAttachment
		};

		const vk::CommandBufferInheritanceInfo inheritanceInfo {
			m_ForwardPass.renderpass, // renderpass
			0u,                       // subpass

			// framebuffer,              // framebuffer
		};

		const vk::CommandBufferBeginInfo primaryCmdBufferBeginInfo {
			{},     // flags
			nullptr // pInheritanceInfo
		};

		// const vk::CommandBufferBeginInfo secondaryCmdBufferBeginInfo {
		// 	vk::CommandBufferUsageFlagBits::eRenderPassContinue, // flags
		// 	&inheritanceInfo                                     // pInheritanceInfo
		// };

		// Record commnads @todo multi-thread secondary commands
		const auto primaryCmd = m_ForwardPass.cmdBuffers[m_CurrentFrame].primary;
		// const auto secondaryCmds = m_ForwardPass.cmdBuffers[m_CurrentFrame].secondaries[m_CurrentFrame];

		primaryCmd.reset();
		primaryCmd.begin(primaryCmdBufferBeginInfo);

		// Transition image to color write
		vk::ImageMemoryBarrier imageMemoryBarrier {
			{},                                        // srcAccessMask
			vk::AccessFlagBits::eColorAttachmentWrite, // dstAccessMask
			vk::ImageLayout::eUndefined,               // oldLayout
			vk::ImageLayout::eColorAttachmentOptimal,  // newLayout
			{},
			{},
			m_Images[imageIndex], // image

			/* subresourceRange */
			vk::ImageSubresourceRange {
			    vk::ImageAspectFlagBits::eColor,
			    0u,
			    1u,
			    0u,
			    1u,
			},
		};

		primaryCmd.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTopOfPipe,
		    vk::PipelineStageFlagBits::eColorAttachmentOutput,
		    {},
		    {},
		    {},
		    imageMemoryBarrier);

		primaryCmd.beginRendering(renderingInfo);

		const vk::DescriptorSet descriptorSet = m_ForwardPass.descriptorSets[m_CurrentFrame];

		// secondaryCmds.reset();

		// secondaryCmds.begin(secondaryCmdBufferBeginInfo);
		primaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_FramePipelineLayout, 0ul, 1ul, &frame.descriptorSet, 0ul, nullptr);
		primaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_ForwardPass.pipelineLayout, 1ul, 1ul, &m_ForwardPass.descriptorSets[m_CurrentFrame], 0ul, nullptr);

		VkDeviceSize offset { 0 };

		vk::Pipeline currentPipeline = VK_NULL_HANDLE;
		uint32_t primitives          = 0;
		scene->GetRegistry()->group(entt::get<TransformComponent, StaticMeshRendererComponent>).each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
			// bind pipeline
			vk::Pipeline newPipeline = renderComp.material->base->shader->pipeline;
			if (currentPipeline != newPipeline)
			{
				primaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, newPipeline);
				currentPipeline = newPipeline;
			}

			// bind buffers
			primaryCmd.bindVertexBuffers(0, 1, renderComp.model->vertexBuffer->GetBuffer(), &offset);
			primaryCmd.bindIndexBuffer(*(renderComp.model->indexBuffer->GetBuffer()), 0u, vk::IndexType::eUint32);

			// draw primitves
			for (const auto* node : renderComp.model->nodes)
			{
				for (const auto& primitive : node->mesh)
				{
					uint32_t textureIndex = renderComp.model->materialParameters[primitive.materialIndex].albedoTextureIndex;
					primaryCmd.drawIndexed(primitive.indexCount, 1ull, primitive.firstIndex, 0ull, textureIndex); // @todo: lol fix this garbage
				}
			}
		});

		// primaryCmd.executeCommands(1ul, &primaryCmd);
		primaryCmd.endRendering();

		colorAttachmentInfo = {
			m_ColorImageView,                         // imageView
			vk::ImageLayout::eColorAttachmentOptimal, // imageLayout

			vk::ResolveModeFlagBits::eAverage,        // resolveMode
			m_ImageViews[imageIndex],                 // resolveImageView
			vk::ImageLayout::eColorAttachmentOptimal, // resolveImageLayout

			vk::AttachmentLoadOp::eLoad,   // loadOp
			vk::AttachmentStoreOp::eStore, // storeOp
		};
		renderingInfo = {
			{}, // flags

			/* renderArea */
			vk::Rect2D {
			    { 0, 0 },                                 // offset
			    m_SurfaceInfo.capabilities.currentExtent, // extent
			},

			1u,                   // layerCount
			{},                   // viewMask
			1u,                   // colorAttachmentCount
			&colorAttachmentInfo, // pColorAttachments
			{},                   // pDepthAttachment
			{},                   // pStencilAttachment
		};

		primaryCmd.beginRendering(renderingInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), primaryCmd);
		primaryCmd.endRendering();

		// Transition color image to present optimal
		imageMemoryBarrier = {
			vk::AccessFlagBits::eColorAttachmentWrite, // srcAccessMask
			{},                                        // dstAccessMask
			vk::ImageLayout::eColorAttachmentOptimal,  // oldLayout
			vk::ImageLayout::ePresentSrcKHR,           // newLayout
			{},
			{},
			m_Images[imageIndex], // image

			/* subresourceRange */
			vk::ImageSubresourceRange {
			    vk::ImageAspectFlagBits::eColor,
			    0u,
			    1u,
			    0u,
			    1u,
			},
		};

		primaryCmd.pipelineBarrier(
		    vk::PipelineStageFlagBits::eColorAttachmentOutput,
		    vk::PipelineStageFlagBits::eBottomOfPipe,
		    {},
		    {},
		    {},
		    imageMemoryBarrier);


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

		vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
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
		catch (vk::OutOfDateKHRError err) // OutOfDateKHR is not considered a success value and throws an error (presentKHR)
		{
			m_LogicalDevice.waitIdle();
			m_SwapchainInvalidated = true;
			return;
		}
	}

	// Increment frame index
	m_CurrentFrame = (m_CurrentFrame + 1u) % MAX_FRAMES_IN_FLIGHT;
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

	m_QueueInfo.graphicsQueue.submit(submitInfo, m_UploadContext.fence);
	VKC(m_LogicalDevice.waitForFences(m_UploadContext.fence, true, UINT_MAX));
	m_LogicalDevice.resetFences(m_UploadContext.fence);

	m_LogicalDevice.resetCommandPool(m_CommandPool, {});
}

#include "Graphics/Renderer.hpp"

#include "Core/Window.hpp"
#include "Graphics/Types.hpp"
#include "Scene/Scene.hpp"
#include "Utils/CVar.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <imgui.h>

Renderer::Renderer(const Renderer::CreateInfo& info)
    : m_LogicalDevice(info.deviceContext.logicalDevice)
    , m_Allocator(info.deviceContext.allocator)
    , m_DepthFormat(info.deviceContext.depthFormat)

{
	CreateSyncObjects();
	CreateDescriptorPools();
	RecreateSwapchainResources(info.window, info.deviceContext);
}

Renderer::~Renderer()
{
	DestroySwapchain();

	m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool, nullptr);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_LogicalDevice.destroyFence(m_RenderFences[i], nullptr);
		m_LogicalDevice.destroySemaphore(m_RenderSemaphores[i], nullptr);
		m_LogicalDevice.destroySemaphore(m_PresentSemaphores[i], nullptr);
	}

	m_LogicalDevice.destroyFence(m_UploadContext.fence);
}

void Renderer::RecreateSwapchainResources(Window* window, DeviceContext deviceContext)
{
	DestroySwapchain();

	m_SurfaceInfo = deviceContext.surfaceInfo;
	m_QueueInfo   = deviceContext.queueInfo;
	m_SampleCount = deviceContext.maxSupportedSampleCount;

	CreateSwapchain();
	CreateCommandPool();
	InitializeImGui(deviceContext, window);

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
	m_LogicalDevice.destroyCommandPool(m_UploadContext.cmdPool);
	m_LogicalDevice.destroyCommandPool(m_CommandPool);

	m_LogicalDevice.resetDescriptorPool(m_DescriptorPool);

	for (const auto& imageView : m_SwapchainImageViews)
	{
		m_LogicalDevice.destroyImageView(imageView);
	}

	m_LogicalDevice.destroySwapchainKHR(m_Swapchain);
};

void Renderer::CreateSyncObjects()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo {};

		vk::FenceCreateInfo fenceCreateInfo {
			vk::FenceCreateFlagBits::eSignaled, // flags
		};

		m_RenderFences[i]      = m_LogicalDevice.createFence(fenceCreateInfo, nullptr);
		m_RenderSemaphores[i]  = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
		m_PresentSemaphores[i] = m_LogicalDevice.createSemaphore(semaphoreCreateInfo, nullptr);
	}

	m_UploadContext.fence = m_LogicalDevice.createFence({}, nullptr);
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

	m_DescriptorPool = m_LogicalDevice.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
}

void Renderer::CreateSwapchain()
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
		vk::ImageUsageFlagBits::eColorAttachment,                                    // imageUsage
		sameQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
		sameQueueIndex ? 0u : 2u,                                                    // queueFamilyIndexCount
		sameQueueIndex ? nullptr : &m_QueueInfo.graphicsQueueIndex,                  // pQueueFamilyIndices
		m_SurfaceInfo.capabilities.currentTransform,                                 // preTransform
		vk::CompositeAlphaFlagBitsKHR::eOpaque,                                      // compositeAlpha -> No alpha-blending between multiple windows
		m_SurfaceInfo.presentMode,                                                   // presentMode
		VK_TRUE,                                                                     // clipped -> Don't render the obsecured pixels
		VK_NULL_HANDLE,                                                              // oldSwapchain
	};

	m_Swapchain       = m_LogicalDevice.createSwapchainKHR(swapchainCreateInfo, nullptr);
	m_SwapchainImages = m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain);

	for (uint32_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                          // flags
			m_SwapchainImages[i],        // image
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

		m_SwapchainImageViews.push_back(m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr));
	}


	for (uint32_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		std::string imageName("swap_chain Image #" + std::to_string(i));
		m_LogicalDevice.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)m_SwapchainImages[i],
		    imageName.c_str(),
		});

		std::string viewName("swap_chain ImageView #" + std::to_string(i));
		m_LogicalDevice.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
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
		m_QueueInfo.graphicsQueueIndex,                     // queueFamilyIndex
	};

	m_CommandPool           = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);
	m_UploadContext.cmdPool = m_LogicalDevice.createCommandPool(commandPoolCreateInfo, nullptr);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo {
		m_CommandPool,                    // commandPool
		vk::CommandBufferLevel::ePrimary, // level
		MAX_FRAMES_IN_FLIGHT,             // commandBufferCount
	};
	m_CommandBuffers = m_LogicalDevice.allocateCommandBuffers(cmdBufferAllocInfo);

	vk::CommandBufferAllocateInfo uploadContextCmdBufferAllocInfo {
		m_UploadContext.cmdPool,
		vk::CommandBufferLevel::ePrimary,
		1u,
	};
	m_UploadContext.cmdBuffer = m_LogicalDevice.allocateCommandBuffers(uploadContextCmdBufferAllocInfo)[0];
}

void Renderer::InitializeImGui(const DeviceContext& deviceContext, Window* window)
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

void Renderer::BuildRenderGraph(const DeviceContext& deviceContext, RenderGraph::BuildInfo buildInfo)
{
	m_RenderGraph.Init({
	    .swapchainExtent     = m_SurfaceInfo.capabilities.maxImageExtent,
	    .descriptorPool      = m_DescriptorPool,
	    .logicalDevice       = deviceContext.logicalDevice,
	    .physicalDevice      = deviceContext.physicalDevice,
	    .allocator           = m_Allocator,
	    .commandPool         = m_CommandPool,
	    .colorFormat         = m_SurfaceInfo.format.format,
	    .depthFormat         = m_DepthFormat,
	    .queueInfo           = m_QueueInfo,
	    .swapchainImages     = m_SwapchainImages,
	    .swapchainImageViews = m_SwapchainImageViews,
	});

	m_RenderGraph.Build(buildInfo);
}

void Renderer::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
	CVar::DrawImguiEditor();
}

void Renderer::DrawScene(Scene* scene)
{
	if (m_SwapchainInvalidated)
		return;

	VKC(m_LogicalDevice.waitForFences(1u, &m_RenderFences[m_CurrentFrame], VK_TRUE, UINT64_MAX));
	VKC(m_LogicalDevice.resetFences(1u, &m_RenderFences[m_CurrentFrame]));

	uint32_t imageIndex;
	vk::Result result = m_LogicalDevice.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_RenderSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
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

	static uint32_t counter = 0u;

	auto cmd = m_CommandBuffers[m_CurrentFrame];

	m_RenderGraph.Render({
	    .graph = &m_RenderGraph,
	    .pass  = nullptr,
	    .scene = scene,

	    .logicalDevice = m_LogicalDevice,
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

	VKC(m_QueueInfo.graphicsQueue.submit(1u, &submitInfo, signalFence));
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
		vk::Result result = m_QueueInfo.presentQueue.presentKHR(presentInfo);
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

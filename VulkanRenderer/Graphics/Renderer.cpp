#include "Graphics/Renderer.hpp"

#include "Core/Window.hpp"
#include "Graphics/Types.hpp"
#include "Scene/Scene.hpp"
#include "Utils/CVar.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <imgui.h>

struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 viewPos;
};

// @todo:
struct SceneData
{
	glm::vec3 lightPos;
};

void GraphUpdate(const RenderGraph::UpdateData& data)
{
	LOG(trace, "Updating graph - camera - {}", data.frameIndex);
	data.scene->GetRegistry()->group(entt::get<TransformComponent, CameraComponent>).each([&](TransformComponent& transformComp, CameraComponent& cameraComp) {
		*(CameraData*)data.graph.MapDescriptorBuffer("frame_data", data.frameIndex) = {
			.projection = cameraComp.GetProjection(),
			.view       = cameraComp.GetView(transformComp.translation),
			.viewPos    = transformComp.translation,
		};
	});

	LOG(trace, "Updating graph - scene - {}", data.frameIndex);
	data.scene->GetRegistry()->group(entt::get<TransformComponent, LightComponent>).each([&](TransformComponent& trasnformComp, LightComponent& lightComp) {
		SceneData* sceneData = (SceneData*)data.graph.MapDescriptorBuffer("scene_data", data.frameIndex);

		LOG(err, "3");
		sceneData[0] = {
			.lightPos = trasnformComp.translation,
		};
		LOG(err, "4");
	});
}

void ForwardPassUpdate(const RenderPass::UpdateData& data)
{
	vk::RenderingAttachmentInfo info;
	LOG(trace, "Updating pass - get - {}", data.frameIndex);
	RenderPass& renderPass    = data.renderPass;
	const uint32_t frameIndex = data.frameIndex;
	vk::Device logicalDevice  = data.logicalDevice;
	Scene* scene              = data.scene;

	LOG(trace, "Updating pass - getset - {}", data.frameIndex);

	vk::DescriptorSet* descriptorSet = &renderPass.descriptorSets[frameIndex];

	LOG(trace, "Updating pass - entt - {}", data.frameIndex);
	// @todo: don't update every frame
	data.scene->GetRegistry()
	    ->group(entt::get<TransformComponent, StaticMeshRendererComponent>)
	    .each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
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
					        *descriptorSet,
					        0ul,
					        albedoTextureIndex,
					        1ul,
					        vk::DescriptorType::eCombinedImageSampler,
					        &albedoTexture->descriptorInfo,
					        nullptr,
					        nullptr,
					    },

					    vk::WriteDescriptorSet {
					        *descriptorSet,
					        0ul,
					        metallicRoughnessTextureIndex,
					        1ul,
					        vk::DescriptorType::eCombinedImageSampler,
					        &metallicRoughnessTexture->descriptorInfo,
					        nullptr,
					        nullptr,
					    },

					    vk::WriteDescriptorSet {
					        *descriptorSet,
					        0ul,
					        normalTextureIndex,
					        1ul,
					        vk::DescriptorType::eCombinedImageSampler,
					        &normalTexture->descriptorInfo,
					        nullptr,
					        nullptr,
					    }
				    };

				    logicalDevice.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0ull, nullptr);
			    }
		    }
	    });
}

void ForwardPassRender(const RenderPass::RenderData& data)
{
	VkDeviceSize offset { 0 };
	vk::Pipeline currentPipeline = VK_NULL_HANDLE;

	data.scene->GetRegistry()->group(entt::get<TransformComponent, StaticMeshRendererComponent>).each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
		const auto cmd = data.cmd;

		// bind pipeline
		vk::Pipeline newPipeline = renderComp.material->base->shader->pipeline;
		if (currentPipeline != newPipeline)
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, newPipeline);
			currentPipeline = newPipeline;
		}
		// bind buffers
		cmd.bindVertexBuffers(0, 1, renderComp.model->vertexBuffer->GetBuffer(), &offset);
		cmd.bindIndexBuffer(*(renderComp.model->indexBuffer->GetBuffer()), 0u, vk::IndexType::eUint32);

		// draw primitves
		for (const auto* node : renderComp.model->nodes)
		{
			for (const auto& primitive : node->mesh)
			{
				uint32_t textureIndex = renderComp.model->materialParameters[primitive.materialIndex].albedoTextureIndex;
				cmd.drawIndexed(primitive.indexCount, 1ull, primitive.firstIndex, 0ull, textureIndex); // @todo: lol fix this garbage
			}
		}
	});
}


Renderer::Renderer(const Renderer::CreateInfo& info)
    : m_LogicalDevice(info.deviceContext.logicalDevice)
    , m_Allocator(info.deviceContext.allocator)
    , m_DepthFormat(info.deviceContext.depthFormat)
    , m_DefaultTexture(info.defaultTexture)
    , m_SkyboxTexture(info.skyboxTexture)

{
	// Validate create info
	ASSERT(info.window && info.skyboxTexture && info.defaultTexture,
	       "Incomplete renderer create info");

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
	CreateImageViews();
	CreateCommandPool();

	m_RenderGraph.Init({
	    .swapchainImageCount = static_cast<uint32_t>(m_SwapchainImages.size()),
	    .swapchainExtent     = m_SurfaceInfo.capabilities.maxImageExtent,
	    .descriptorPool      = m_DescriptorPool,
	    .logicalDevice       = deviceContext.logicalDevice,
	    .physicalDevice      = deviceContext.physicalDevice,
	    .allocator           = m_Allocator,
	    .commandPool         = m_CommandPool,
	    .colorFormat         = m_SurfaceInfo.format.format,
	    .depthFormat         = m_DepthFormat,
	    .queueInfo           = m_QueueInfo,
	});


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

	RenderPassRecipe uiPassRecipe;
	RenderPassRecipe forwardPassRecipe;

	RenderPassRecipe::AttachmentInfo colorTarget = {};

	forwardPassRecipe
	    .SetName("forward")
	    .AddColorOutput({
	        .name     = "forwardpass",
	        .size     = { 1.0, 1.0 },
	        .sizeType = RenderPassRecipe::SizeType::eSwapchainRelative,
	        .format   = m_SurfaceInfo.format.format,
	        .samples  = m_SampleCount,
	    })
	    .SetDepthStencilOutput({
	        .name     = "forward_depth",
	        .size     = { 1.0, 1.0 },
	        .sizeType = RenderPassRecipe::SizeType::eSwapchainRelative,
	        .format   = m_DepthFormat,
	        .samples  = m_SampleCount,
	    })
	    .AddTextureInput({
	        .name           = "texture_cubes",
	        .binding        = 1,
	        .count          = 8u,
	        .type           = vk::DescriptorType::eCombinedImageSampler,
	        .stageMask      = vk::ShaderStageFlagBits::eFragment,
	        .defaultTexture = m_DefaultTexture,
	    })
	    .AddTextureInput({
	        .name           = "texture_2ds",
	        .binding        = 0,
	        .count          = 32,
	        .type           = vk::DescriptorType::eCombinedImageSampler,
	        .stageMask      = vk::ShaderStageFlagBits::eFragment,
	        .defaultTexture = m_DefaultTexture,
	    })
	    .SetUpdateAction(&ForwardPassUpdate)
	    .SetRenderAction(&ForwardPassRender);

	uiPassRecipe
	    .SetName("ui")
	    .AddColorOutput({
	        .name     = "backbuffer",
	        .size     = { 1.0, 1.0 },
	        .sizeType = RenderPassRecipe::SizeType::eSwapchainRelative,
	        .format   = m_SurfaceInfo.format.format,
	        .samples  = m_SampleCount,
	        .input    = "forwardpass",
	    })
	    .SetUpdateAction([](const RenderPass::UpdateData& data) {
		    LOG(trace, "no op");
	    })
	    .SetRenderAction([](const RenderPass::RenderData& data) {
		    ImGui::Render();
		    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), data.cmd);
	    });

	m_RenderGraph
	    .SetName("SimpleGraph")
	    .SetBackbuffer("backbuffer")
	    .AddBufferInput({
	        .name      = "frame_data",
	        .binding   = 0,
	        .count     = 1,
	        .type      = vk::DescriptorType::eUniformBuffer,
	        .stageMask = vk::ShaderStageFlagBits::eVertex,

	        .size        = (sizeof(glm::mat4) * 2) + (sizeof(glm::vec4)),
	        .initialData = nullptr,
	    })
	    .AddBufferInput({
	        .name      = "scene_data",
	        .binding   = 1,
	        .count     = 1,
	        .type      = vk::DescriptorType::eUniformBuffer,
	        .stageMask = vk::ShaderStageFlagBits::eVertex,

	        .size        = sizeof(glm::vec4),
	        .initialData = nullptr,
	    })
	    .AddRenderPassRecipe(forwardPassRecipe)
	    .AddRenderPassRecipe(uiPassRecipe)
	    .SetUpdateAction(&GraphUpdate)
	    .Build();

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

	m_LogicalDevice.destroyImageView(m_DepthTargetView);
	m_LogicalDevice.destroyImageView(m_ColorTargetView);

	m_Allocator.destroyImage(m_DepthTarget, m_DepthTarget);
	m_Allocator.destroyImage(m_ColorTarget, m_ColorTarget);

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
		{},                                      // flags
		100,                                     // maxSets
		static_cast<uint32_t>(poolSizes.size()), // poolSizeCount
		poolSizes.data(),                        // pPoolSizes
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

	m_Swapchain       = m_LogicalDevice.createSwapchainKHR(swapchainCreateInfo, nullptr);
	m_SwapchainImages = m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain);
}

void Renderer::CreateImageViews()
{
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
}

void Renderer::CreateResolveColorImage()
{
}

void Renderer::CreateDepthImage()
{
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

	vma::AllocationCreateInfo imageAllocInfo({}, vma::MemoryUsage::eGpuOnly, vk::MemoryPropertyFlagBits::eDeviceLocal);

	m_DepthTarget = m_Allocator.createImage(imageCreateInfo, imageAllocInfo);

	// Create depth image-view
	vk::ImageViewCreateInfo imageViewCreateInfo {
		{},                     // flags
		m_DepthTarget,          // image
		vk::ImageViewType::e2D, // viewType
		m_DepthFormat,          // format

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

	m_DepthTargetView = m_LogicalDevice.createImageView(imageViewCreateInfo, nullptr);
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

	auto cmd = m_CommandBuffers[m_CurrentFrame];
	cmd.reset();
	cmd.begin(vk::CommandBufferBeginInfo {});


	m_RenderGraph.Update(scene, m_CurrentFrame);
	m_RenderGraph.Render({
	    scene,
	    cmd,
	    m_CurrentFrame,
	});

	SetupPresentBarriers(cmd, imageIndex);
	cmd.end();

	SubmitQueue(m_PresentSemaphores[m_CurrentFrame], m_RenderSemaphores[m_CurrentFrame], m_RenderFences[m_CurrentFrame], cmd);
	PresentFrame(m_RenderSemaphores[m_CurrentFrame], imageIndex);

	m_CurrentFrame = (m_CurrentFrame + 1u) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::SetupPresentBarriers(vk::CommandBuffer cmd, uint32_t imageIndex)
{
	// Transition color image to present optimal
	vk::ImageMemoryBarrier imageMemoryBarrier = {
		vk::AccessFlagBits::eColorAttachmentWrite, // srcAccessMask
		{},                                        // dstAccessMask
		vk::ImageLayout::eColorAttachmentOptimal,  // oldLayout
		vk::ImageLayout::ePresentSrcKHR,           // newLayout
		{},
		{},
		m_SwapchainImages[imageIndex], // image

		/* subresourceRange */
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eColor,
		    0u,
		    1u,
		    0u,
		    1u,
		},
	};

	cmd.pipelineBarrier(
	    vk::PipelineStageFlagBits::eColorAttachmentOutput,
	    vk::PipelineStageFlagBits::eBottomOfPipe,
	    {},
	    {},
	    {},
	    imageMemoryBarrier);
}

void Renderer::SubmitQueue(vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, vk::Fence signalFence, vk::CommandBuffer cmd)
{
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

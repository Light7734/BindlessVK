#include "Graphics/Renderer.h"

#include "Core/Window.h"
#include "Graphics/Device.h"
#include "Graphics/RendererCommand.h"
#include "Graphics/Shader.h"
#include "Graphics/Swapchain.h"

#include <GLFW/glfw3.h>

Renderer::Renderer(class Window* window, uint32_t maxConcurrentFrames)
    : m_Window(window)
    , m_MaxConcurrentFrames(maxConcurrentFrames)
{
	ASSERT(window, "Null window passed to renderer");
	m_Device = new Device(window);

	RendererCommand::Init(m_Device);

	ASSERT(m_Device, "Failed to create vulkan device");
	m_Swapchain = new Swapchain(window, m_Device);

	CreateSyncObjects();

	m_QuadRendererProgram = std::make_unique<QuadRendererProgram>(m_Device, m_Swapchain->GetRenderPass(), m_Swapchain->GetExtent(), m_Swapchain->GetImageCount());
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_Device->logical());

	// destroy renderer programs
	m_QuadRendererProgram.reset();

	delete m_Swapchain;
	delete m_Device;
}

void Renderer::BeginFrame()
{
}

void Renderer::BeginScene()
{
	m_QuadRendererProgram->Map();
}

void Renderer::DrawQuad(const glm::mat4& transform, const glm::vec4& tint)
{
	// Dynamic rainbow colors >_<
	// #note: this is not an actual part of the draw quad process, it's just a fun thing to have
	const double time = glfwGetTime() * 3.0f;
	const float r     = (((1.0f - sin(time / 2.0)) / 2.0f)) + abs(tan(time / 15.0f));
	const float g     = (((1.0f + sin(time / 0.5)) / 2.0f)) + abs(tan(time / 15.0f));
	const float b     = (((1.0f + sin(time / 1.0)) / 2.0f)) + abs(tan(time / 15.0f));
	const float t     = (((1.0f + cos(time)) / 2.0f)) + abs(tan(time / 25.0f));

	QuadRendererProgram::Vertex* map = m_QuadRendererProgram->GetMapCurrent();

	// top left
	map[0].position = transform * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
	map[0].tint     = glm::vec4(r, 0.0, t / 0.5, 1.0f);
	map[0].uv       = glm::vec2(0.0f, 0.0f);

	// top right
	map[1].position = transform * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f);
	map[1].tint     = glm::vec4(t / 2.0, g, 0.0f, 1.0f);
	map[1].uv       = glm::vec2(1.0f, 0.0f);

	// bottom right
	map[2].position = transform * glm::vec4(0.5f, 0.5f, 0.0f, 1.0f);
	map[2].tint     = glm::vec4(0.0f, t / 1.0, b, 1.0f);
	map[2].uv       = glm::vec2(1.0f, 1.0f);

	// bottom left
	map[3].position = transform * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f);
	map[3].tint     = glm::vec4(t / 2.0, t, t, 1.0f);
	map[3].uv       = glm::vec2(0.0f, 1.0f);

	if (!m_QuadRendererProgram->TryAdvance())
	{
		LOG(warn, "Quad renderer exceeded max vertices limit!");

		// #todo: flush scene
		EndScene();
	}
}

void Renderer::EndScene()
{
	// unmap vertex/index buffers
	m_QuadRendererProgram->UnMap();

	vkWaitForFences(m_Device->logical(), 1u, &m_Fences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	// fetch image ( & recreate swap chain if necessary )
	uint32_t imageIndex = m_Swapchain->FetchNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);
	if (imageIndex == UINT32_MAX)
	{
		Swapchain* old = m_Swapchain;

		vkDeviceWaitIdle(m_Device->logical());
		m_Swapchain = new Swapchain(m_Window, m_Device, old);
		return;
	}

	// write uniform buffers
	m_QuadRendererProgram->UpdateCamera(imageIndex);

	// check if the image is in use
	if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(m_Device->logical(), 1u, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	// mark the image as now being used by this frame
	m_ImagesInFlight[imageIndex] = m_Fences[m_CurrentFrame];

	// generate command buffers
	VkCommandBuffer QuadRendererCommands = m_QuadRendererProgram->RecordCommandBuffer(m_Swapchain->GetRenderPass(),
	                                                                                  m_Swapchain->GetFramebuffer(imageIndex),
	                                                                                  m_Swapchain->GetExtent(),
	                                                                                  imageIndex);

	std::vector<VkCommandBuffer> commandBuffers;
	commandBuffers.push_back(QuadRendererCommands);

	// submit info
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo {
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount   = 1u,
		.pWaitSemaphores      = &m_ImageAvailableSemaphores[m_CurrentFrame], // <-- (semaphore) wait for the image requested by vkAquireImage to be available
		.pWaitDstStageMask    = &waitStage,
		.commandBufferCount   = static_cast<uint32_t>(commandBuffers.size()),
		.pCommandBuffers      = commandBuffers.data(),
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores    = &m_RenderFinishedSemaphores[m_CurrentFrame],
	};

	// submit queues
	vkResetFences(m_Device->logical(), 1u, &m_Fences[m_CurrentFrame]);
	VKC(vkQueueSubmit(m_Device->graphicsQueue(), 1u, &submitInfo, m_Fences[m_CurrentFrame]));

	VkPresentInfoKHR presentInfo {
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores    = &m_RenderFinishedSemaphores[m_CurrentFrame], // <-- (semaphore) wait for the the frame to be rendererd
		.swapchainCount     = 1u,
		.pSwapchains        = m_Swapchain->GetVkSwapchain(),
		.pImageIndices      = &imageIndex,
		.pResults           = nullptr,
	};

	// increment frame
	m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxConcurrentFrames;

	// finally... present queue
	VkResult result = vkQueuePresentKHR(m_Device->presentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->WasResized())
	{
		m_Window->ResizeHandled();

		vkDeviceWaitIdle(m_Device->logical());
		Swapchain* old = m_Swapchain;
		m_Swapchain    = new Swapchain(m_Window, m_Device, old);

		RendererCommand::Init(m_Device);

		m_QuadRendererProgram.reset();
		m_QuadRendererProgram = std::make_unique<QuadRendererProgram>(m_Device, m_Swapchain->GetRenderPass(), m_Swapchain->GetExtent(), m_Swapchain->GetImageCount());
	}
	else if (result != VK_SUCCESS)
	{
		ASSERT(false, "bruh moment")
	}
}

void Renderer::EndFrame()
{
}

void Renderer::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(m_MaxConcurrentFrames);
	m_RenderFinishedSemaphores.resize(m_MaxConcurrentFrames);
	m_Fences.resize(m_MaxConcurrentFrames);
	m_ImagesInFlight.resize(m_Swapchain->GetImageCount(), VK_NULL_HANDLE);

	// semaphore create-info
	VkSemaphoreCreateInfo semaphoreCreateInfo {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	// fence create-info
	VkFenceCreateInfo fenceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (uint32_t i = 0ull; i < m_MaxConcurrentFrames; i++)
	{
		VKC(vkCreateSemaphore(m_Device->logical(), &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		VKC(vkCreateSemaphore(m_Device->logical(), &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]));

		vkCreateFence(m_Device->logical(), &fenceCreateInfo, nullptr, &m_Fences[i]);
	}
}

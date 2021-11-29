#pragma once

#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/RendererPrograms/QuadRendererProgram.h"

#include <glm/glm.hpp>
#include <volk.h>

struct GLFWwindow;
class Shader;

class Renderer
{
public:
	Renderer(class Window* window, uint32_t maxConcurrentFrames);
	~Renderer();

	void BeginFrame();
	void BeginScene();

	// #todo:
	void DrawQuad(const glm::mat4& transform, const glm::vec4& tint);

	void EndScene();
	void EndFrame();

private:
	class Window* m_Window       = nullptr;
	class Device* m_Device       = nullptr;
	class Swapchain* m_Swapchain = nullptr;

	VkExtent2D m_Extent;

	std::unique_ptr<QuadRendererProgram> m_QuadRendererProgram;

	// synchronization
	const uint32_t m_MaxConcurrentFrames;
	uint32_t m_CurrentFrame = 0u;

	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_Fences;
	std::vector<VkFence> m_ImagesInFlight;

private:
	void CreateSyncObjects();
};
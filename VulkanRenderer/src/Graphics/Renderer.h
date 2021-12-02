#pragma once

#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Model.h"
#include "Graphics/RendererPrograms/ModelRendererProgram.h"
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

	inline bool IsSwapchainOk() const { return m_SwapchainOK; }

	// #todo:
	void DrawQuad(const glm::mat4& transform, const glm::vec4& tint);
	void DrawModel(const glm::mat4& transform, Model& model);

	void EndScene();
	void EndFrame();

	void Resize(int width, int height);

	inline class Device* GetDevice() { return m_Device; }

private:
	bool m_SwapchainOK = true;

	class Window* m_Window       = nullptr;
	class Device* m_Device       = nullptr;
	class Swapchain* m_Swapchain = nullptr;

	VkExtent2D m_Extent;

	std::unique_ptr<QuadRendererProgram> m_QuadRendererProgram;
	std::unique_ptr<ModelRendererProgram> m_ModelRendererProgram;

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
#include "Core/Base.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Debug/UserInterface.h"
#include "Graphics/Renderer.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <imgui.h>

int main()
{
	// init logger
	Logger::Init();

	// create window & pipeline
	Window window     = Window(800, 600);
	Renderer renderer = Renderer(&window, 3);
	Model model(renderer.GetDevice(), "res/viking_room.obj", "res/viking_room.png");

	window.SetWindowUserPointer(&renderer);
	UserInterface userInterface = UserInterface(window.GetHandle(), renderer.GetDevice(), renderer.GetRenderPass());

	// fps calculator
	Timer timer;
	uint32_t frames = 0u;

	ImGuiIO& io = ImGui::GetIO();
	// Upload Fonts
	{
		VkCommandPool commandPool;
		// command pool create-info
		VkCommandPoolCreateInfo commandpoolCreateInfo {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = renderer.GetDevice()->graphicsQueueIndex(),
		};

		VKC(vkCreateCommandPool(renderer.GetDevice()->logical(), &commandpoolCreateInfo, nullptr, &commandPool));
		// Use any command queue
		// command buffer allocate-info
		VkCommandBufferAllocateInfo commandBufferAllocateInfo {
			.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool        = commandPool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkCommandBuffer commandBuffer;
		VKC(vkAllocateCommandBuffers(renderer.GetDevice()->logical(), &commandBufferAllocateInfo, &commandBuffer));

		VKC(vkResetCommandPool(renderer.GetDevice()->logical(), commandPool, 0));
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VKC(vkBeginCommandBuffer(commandBuffer, &begin_info));

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		VkSubmitInfo end_info       = {};
		end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers    = &commandBuffer;
		VKC(vkEndCommandBuffer(commandBuffer));
		VKC(vkQueueSubmit(renderer.GetDevice()->graphicsQueue(), 1, &end_info, VK_NULL_HANDLE));
		VKC(vkDeviceWaitIdle(renderer.GetDevice()->logical()));
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
	/**  main loop  */
	while (!window.IsClosed())
	{
		if (renderer.IsSwapchainOk())
			glfwPollEvents();

		// render user interface
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		static bool showDemo = true;
		ImGui::ShowDemoWindow(&showDemo);

		ImGui::Render();
		ImDrawData* imguiDrawData = ImGui::GetDrawData();

		// render frame
		renderer.BeginScene();
		renderer.DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, -1.0f)), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		renderer.DrawModel(glm::mat4(1.0f), model);
		renderer.EndScene(imguiDrawData);

		// calculate fps
		frames++;
		if (timer.ElapsedTime() >= 1.0f)
		{
			LOG(trace, "FPS: {}", frames);

			timer.Reset();
			frames = 0u;
		}
	}

	return 0;
}

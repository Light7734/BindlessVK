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

	{
	}
	/**  main loop  */
	while (!window.IsClosed())
	{
		if (renderer.IsSwapchainOk())
			glfwPollEvents();

		// user interface ...
		userInterface.Begin();

		userInterface.End();

		// render frame
		renderer.BeginScene();
		renderer.DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, -1.0f)), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		renderer.DrawModel(glm::mat4(1.0f), model);
		renderer.EndScene(userInterface.GetDrawData());

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

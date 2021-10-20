#include "Base.h"

#include "Pipeline.h"
#include "Window.h"

#include "Timer.h"
#include <glfw/glfw3.h>

int main()
{
	Logger::Init();
	Window window = Window();

	Pipeline pipeline(window.GetHandle());

	Timer timer;
	uint32_t frames = 0u;

	while (!window.IsClosed())
	{
		glfwPollEvents();

		pipeline.RenderFrame();

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
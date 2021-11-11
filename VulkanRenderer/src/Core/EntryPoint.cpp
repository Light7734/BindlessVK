#include "Core/Base.h"
#include "Core/Timer.h"
#include "Core/Window.h"

#include "Graphics/Pipeline.h"

#include <glfw/glfw3.h>

int main()
{
	// init logger
	Logger::Init();

	// create window & pipeline
	Window window = Window();
	Pipeline pipeline = Pipeline(window.GetHandle());

	window.RegisterPipeline(&pipeline);

	// fps calculator
	Timer timer;
	uint32_t frames = 0u;

	/**  main loop  */
	while (!window.IsClosed())
	{
		// handle events
		glfwPollEvents();

		// render frame
		pipeline.RenderFrame();

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
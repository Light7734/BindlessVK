#include "Base.h"

#include "DeviceContext.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Window.h"

#include <glfw/glfw3.h>

int main()
{
	Logger::Init();
	Window window = Window();

	DeviceContext deviceContext = DeviceContext(window.GetHandle());
	SharedContext sharedContext = deviceContext.GetSharedContext();

	Shader shader("res/VertexShader.glsl", "res/PixelShader.glsl", deviceContext.GetSharedContext());
	Pipeline pipeline(deviceContext.GetSharedContext(), shader.GetShaderStages());

	float timer = 0.0f;
	float elapsedTime = 0.0f;
	uint32_t frames = 0u;

	while (!window.IsClosed())
	{
		glfwPollEvents();

		uint32_t imageIndex = pipeline.AquireNextImage();
		pipeline.SubmitCommandBuffer(imageIndex);

		frames++;
		if (glfwGetTime() - elapsedTime >= 1.0f)
		{
			LOG(trace, "FPS: {}", frames);

			elapsedTime = glfwGetTime();
			timer = 0.0f;
			frames = 0u;

		}
	}

	vkDeviceWaitIdle(sharedContext.logicalDevice);
	return 0;
}
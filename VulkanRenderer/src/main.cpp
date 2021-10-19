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

	while (!window.IsClosed())
	{
		glfwPollEvents();

		uint32_t imageIndex = pipeline.AquireNextImage();
		pipeline.SubmitCommandBuffer(imageIndex);

		LOG(info, "Event handling not supported yet...");
	}

	vkDeviceWaitIdle(sharedContext.logicalDevice);
	return 0;
}
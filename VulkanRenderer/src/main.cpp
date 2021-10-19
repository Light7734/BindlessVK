#include "Base.h"

#include "DeviceContext.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Window.h"


int main()
{
	Logger::Init();
	Window window = Window();

	DeviceContext deviceContext = DeviceContext(window.GetHandle());

	Shader shader("res/VertexShader.glsl", "res/PixelShader.glsl", deviceContext.GetSharedContext());
	Pipeline pipeline(deviceContext.GetSharedContext(), shader.GetShaderStages());

	while (!window.IsClosed())
	{
		LOG(info, "Event handling not supported yet...");
	}

	return 0;
}
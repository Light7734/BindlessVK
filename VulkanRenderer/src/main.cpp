#include "Base.h"

#include "DeviceContext.h"
#include "Shader.h"
#include "Window.h"

int main()
{
	Logger::Init();

	Window window = Window();
	DeviceContext graphicsContext = DeviceContext(window.GetHandle());

	Shader shader("res/VertexShader.glsl", "res/PixelShader.glsl", graphicsContext.GetSharedContext());

	while (!window.IsClosed())
	{
		LOG(info, "Bruh");
	}

	return 0;
}
#include "Base.h"

#include "GraphicsContext.h"
#include "Shader.h"
#include "Window.h"

int main()
{
	Logger::Init();

	Window window = Window();
	GraphicsContext graphicsContext = GraphicsContext(window.GetHandle());

	Shader shader("res/VertexShader.glsl", "res/PixelShader.glsl", graphicsContext.GetSharedContext());

	while (!window.IsClosed())
	{
		LOG(info, "Bruh");
	}

	return 0;
}
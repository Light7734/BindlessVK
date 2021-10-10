#include "Base.h"

#include "vkGraphicsContext.h"
#include "Window.h"

int main()
{
	Logger::Init();

	Window window = Window();
	vkGraphicsContext gfxContext = vkGraphicsContext(window.GetHandle());

	while (!window.IsClosed())
	{
		LOG(info, "Bruh");
	}

	return 0;
}
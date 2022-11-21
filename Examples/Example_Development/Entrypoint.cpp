#include "Development.hpp"
#include "Framework/Core/Application.hpp"

int main(int argc, char* argv[])
{
	try
	{
		DevelopmentExampleApplication application;

		Timer deltaTimer;
		while (!application.m_Window.ShouldClose())
		{
			application.m_Window.PollEvents();

			application.OnTick(deltaTimer.ElapsedTime());
			deltaTimer.Reset();

			// @todo: Handle swapchain invalidation
		}
	}
	catch (bvk::BindlessVkException bvkException)
	{
		return 1;
	}
	return 0;
}

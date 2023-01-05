#include "Development.hpp"
#include "Framework/Core/Application.hpp"

int main(int argc, char* argv[])
{
	try
	{
		DevelopmentExampleApplication application;

		Timer delta_timer;
		while (!application.m_window.should_close())
		{
			application.m_window.poll_events();

			application.on_tick(delta_timer.elapsed_time());
			delta_timer.reset();

			// @todo: Handle swapchain invalidation
		}
	}
	catch (bvk::BindlessVkException bvkException)
	{
		return 1;
	}
	return 0;
}

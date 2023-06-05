#include "Development.hpp"

#include <Amender/Logger.hpp>

/** The entry point */
int main(int argc, char *argv[])
{
	try
	{
		DevelopmentExampleApplication application = DevelopmentExampleApplication();
		Timer delta_timer;

		while (!application.window.should_close())
		{
			application.window.poll_events();

			application.on_tick(delta_timer.elapsed_time());
			delta_timer.reset();
		}
	}
	catch (Exception exception)
	{
		log_crt("Uncaught exception: {}", exception.what());
		return 1;
	}
	catch (std::exception exception)
	{
		log_crt("Uncaught std::exception: {}", exception.what());
		return 2;
	}

	return 0;
}

#include "Development.hpp"

#include <Amender/Amender.hpp>

/** The entry point */
int main(int argc, char *argv[])
{
	Profiler::initialize("profile.json");
	auto return_code = int { 0 };

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
	}
	catch (std::exception exception)
	{
		log_crt("Uncaught std::exception: {}", exception.what());
	}

	Profiler::terminate();
	return return_code;
}

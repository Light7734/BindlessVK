#include "Development.hpp"

#include <Framework/Common/Common.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
	try {
		DevelopmentExampleApplication application = DevelopmentExampleApplication();
		Timer delta_timer;

		while (!application.window.should_close()) {
			application.window.poll_events();

			application.on_tick(delta_timer.elapsed_time());
			delta_timer.reset();

			// @todo: Handle swapchain invalidation
		}
	}
	catch (bvk::BindlessVkException bvkException) {
		std::cout << "Bvk exception failed: " << bvkException.what() << std::endl;
		return 1;
	}
	catch (std::exception exception) {
		std::cout << "exception failed: " << exception.what() << std::endl;
	}
	return 0;
}

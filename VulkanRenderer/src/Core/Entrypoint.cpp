#include "Core/Base.hpp"
#include "Core/Timer.hpp"

#include <iostream>

int main()
{
	int exitCode = 0;

	// Initialize Logger (assumed to always succeed)
	Logger::Init();

	// Try: Run the application
	try
	{
		// Initialize singletons...


		// Main loop...
		uint32_t frames = 0u;
		Timer fpsTimer;
		while (true)
		{
			frames++;

			if (fpsTimer.ElapsedTime() >= 1.0f)
			{
				LOG(trace, "FPS: {}", frames);
				frames = 0;
				fpsTimer.Reset();
			}
		}
	}

	// Report unexpected termination
	catch (FailedAsssertionException exception)
	{
		LOG(critical, "Terminating due FailedAssertionException");
		exitCode = 1;
	}

	// Cleanup...

	// Return
	return exitCode;
}

#include "Core/Base.hpp"

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
		while (true)
		{
			LOG(trace, "looping...");
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

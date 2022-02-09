#include "Core/Base.hpp"
#include "Core/Window.hpp"
#include "Graphics/Device.hpp"
#include "Utils/Timer.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

int main()
{
	int exitCode = 0;

	// Initialize Logger (assumed to always succeed)
	Logger::Init();

	// Try: Run the application
	try
	{
		// Create window
		WindowCreateInfo windowCreateInfo {
			.specs = {
			    .title  = "Vulkan renderer",
			    .width  = 800u,
			    .height = 600u,
			},
			.hints = {
			    { GLFW_CLIENT_API, GLFW_NO_API },
			    { GLFW_RESIZABLE, GLFW_FALSE },
			    { GLFW_FLOATING, GLFW_TRUE },
			    { GLFW_VISIBLE, GLFW_FALSE },
			},
		};

		Window window(windowCreateInfo);

		// Set required device extensions
		auto deviceExtensions = window.GetRequiredExtensions();
		deviceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// Create device
		DeviceCreateInfo deviceCreateInfo {
			.layers             = { "VK_LAYER_KHRONOS_validation" },
			.extensions         = deviceExtensions,
			.enableDebugging    = true,
			.minMessageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
			.messageTypes       = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		};

		Device device(deviceCreateInfo, window);

		// Main loop...
		uint32_t frames = 0u;
		Timer fpsTimer;
		while (!window.ShouldClose())
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

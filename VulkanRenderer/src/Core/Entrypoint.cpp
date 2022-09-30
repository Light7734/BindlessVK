#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Core/Window.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Shader.hpp"
#include "Utils/Timer.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

int main()
{
	Logger::Init();

	int exitCode = 0;
	try
	{
		/////////////////////////////////////////////////////////////////////////////////
		// Create objects

		// window
		WindowCreateInfo windowCreateInfo {
			.specs = {
			    .title  = "Vulkan renderer",
			    .width  = 800u,
			    .height = 600u,
			},
			.hints = {
			    { GLFW_CLIENT_API, GLFW_NO_API },
			    { GLFW_FLOATING, GLFW_TRUE },
			},
		};
		Window window(windowCreateInfo);


		// device
		auto deviceExtensions = window.GetRequiredExtensions();
		deviceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		DeviceCreateInfo deviceCreateInfo {
			.window                  = &window,
			.layers                  = { "VK_LAYER_KHRONOS_validation" },
			.instanceExtensions      = deviceExtensions,
			.logicalDeviceExtensions = {
			    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			},
			.enableDebugging      = true,
			.debugMessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
			.debugMessageTypes    = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		};
		Device device(deviceCreateInfo);

		// renderer
		RendererCreateInfo rendererCreateInfo {
			.window        = &window,
			.deviceContext = device.GetContext(),
		};
		std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>(rendererCreateInfo);

		/////////////////////////////////////////////////////////////////////////////////
		// Main loop
		uint32_t frames = 0u;
		Timer fpsTimer;
		while (!window.ShouldClose())
		{
			// Window events
			window.PollEvents();

			renderer->BeginFrame();
			renderer->EndFrame();

			// Re-create the renderer
			if (renderer->IsSwapchainInvalidated())
			{
				renderer.reset();

				// renderer
				RendererCreateInfo rendererCreateInfo {
					.window        = &window,
					.deviceContext = device.GetContext(),
				};
				renderer = std::make_unique<Renderer>(rendererCreateInfo);
			}

			// FPS Counter
			frames++;
			if (fpsTimer.ElapsedTime() >= 1.0f)
			{
				LOG(trace, "FPS: {}", frames);
				frames = 0;
				fpsTimer.Reset();
			}
		}
	}
	catch (FailedAsssertionException exception)
	{
		LOG(critical, "Terminating due FailedAssertionException");
		exitCode = 1;
	}

	// Cleanup...
	return exitCode;
}

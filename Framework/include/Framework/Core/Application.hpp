#pragma once

#include "BindlessVk/MaterialSystem.hpp"
#include "BindlessVk/Model.hpp"
#include "BindlessVk/Renderer.hpp"
#include "BindlessVk/Texture.hpp"
#include "Framework/Core/Common.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"

#include <GLFW/glfw3.h>

class Application
{
public:
	Application();
	~Application();

	static VkBool32 VulkanDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                           void* pUserData);

	virtual void OnTick(double deltaTime) = 0;
	virtual void OnSwapchainRecreate()    = 0;

public:
	Window m_Window;
	Scene m_Scene;

	bvk::DeviceSystem m_DeviceSystem;
	bvk::Renderer m_Renderer;
	bvk::TextureSystem m_TextureSystem;
	bvk::MaterialSystem m_MaterialSystem;
	bvk::ModelSystem m_ModelSystem;

	CameraController m_CameraController;

	std::vector<const char*> m_InstanceExtensions = {};
	std::vector<const char*> m_DeviceExtensions   = {};

protected:
	uint64_t m_MessengerWarnCount = {};
	uint64_t m_MessengerErrCount  = {};
};

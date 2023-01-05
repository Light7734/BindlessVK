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

	static VkBool32 vulkan_debug_message_callback(
	  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	  VkDebugUtilsMessageTypeFlagsEXT message_types,
	  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	  void* user_data
	);

	virtual void on_tick(double delta_time) = 0;
	virtual void on_swapchain_recreate()    = 0;

public:
	Window m_window;
	Scene m_scene;

	bvk::DeviceSystem m_device_system;
	bvk::Renderer m_renderer;
	bvk::TextureSystem m_texture_system;
	bvk::MaterialSystem m_material_system;
	bvk::ModelSystem m_model_system;

	CameraController m_camera_controller;

	std::vector<const char*> m_instance_extensions = {};
	std::vector<const char*> m_device_extensions   = {};

protected:
	uint64_t m_messenger_warn_count = {};
	uint64_t m_messenger_err_count  = {};
};

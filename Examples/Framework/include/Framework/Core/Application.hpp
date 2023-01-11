#pragma once

#include "BindlessVk/MaterialSystem.hpp"
#include "BindlessVk/ModelLoader.hpp"
#include "BindlessVk/Renderer.hpp"
#include "BindlessVk/Texture.hpp"
#include "Framework/Core/Common.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/StagingPool.hpp"

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
	Window window;
	Scene scene;

	bvk::DeviceSystem device_system;
	bvk::Renderer renderer;
	bvk::TextureSystem texture_system;
	bvk::MaterialSystem material_system;
	bvk::ModelLoader model_loader;

	std::unordered_map<uint64_t, bvk::Model> models;

	CameraController camera_controller;

	std::vector<const char*> instance_extensions = {};
	std::vector<const char*> device_extensions   = {};

	StagingPool staging_pool = {};

protected:
	uint64_t messenger_warn_count = {};
	uint64_t messenger_err_count  = {};
};

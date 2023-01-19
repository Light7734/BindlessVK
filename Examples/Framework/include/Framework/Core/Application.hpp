#pragma once

#include "Framework/Common/Common.hpp"
//

#include "BindlessVk/MaterialSystem.hpp"
#include "BindlessVk/ModelLoader.hpp"
#include "BindlessVk/Renderer.hpp"
#include "BindlessVk/Texture.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Pools/StagingPool.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/Logger.hpp"

#include <GLFW/glfw3.h>

class Application
{
public:
	Application();
	virtual ~Application();

	static VkBool32 vulkan_debug_message_callback(
	  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	  VkDebugUtilsMessageTypeFlagsEXT message_types,
	  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	  void* user_data
	);

	virtual void on_tick(double delta_time) = 0;
	virtual void on_swapchain_recreate()    = 0;

public:
	Window window = {};
	Scene scene   = {};

	bvk::DeviceSystem device_system     = {};
	bvk::Renderer renderer              = {};
	bvk::TextureLoader texture_loader   = {};
	bvk::MaterialSystem material_system = {};
	bvk::ModelLoader model_loader       = {};

	map<u64, bvk::Model> models     = {};
	map<u64, bvk::Texture> textures = {};

	CameraController camera_controller = {};

	vec<c_str> instance_extensions = {};
	vec<c_str> device_extensions   = {};

	StagingPool staging_pool = {};

	Logger logger;

protected:
	u64 messenger_warn_count = {};
	u64 messenger_err_count  = {};
};

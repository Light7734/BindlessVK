#pragma once

#include "Framework/Common/Common.hpp"
//

#include "BindlessVk/MaterialSystem.hpp"
#include "BindlessVk/ModelLoader.hpp"
#include "BindlessVk/Renderer.hpp"
#include "BindlessVk/Shader.hpp"
#include "BindlessVk/ShaderLoader.hpp"
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

	virtual void on_tick(double delta_time) = 0;
	virtual void on_swapchain_recreate() = 0;

public:
	Logger logger = Logger();

	Window window = {};
	Scene scene = {};

	bvk::VkContext vk_context = {};
	bvk::Renderer renderer = {};

	bvk::TextureLoader texture_loader = {};
	bvk::ModelLoader model_loader = {};
	bvk::ShaderLoader shader_loader = {};

	vk::DescriptorPool descriptor_pool = {};

	hash_map<u64, bvk::Model> models = {};
	hash_map<u64, bvk::Texture> textures = {};
	hash_map<u64, bvk::Shader> shaders = {};
	hash_map<u64, bvk::ShaderEffect> shader_effects = {};
	hash_map<u64, bvk::ShaderEffect::Configuration> shader_effect_configurations = {};
	hash_map<u64, bvk::Material> materials = {};

	CameraController camera_controller = {};

	vec<c_str> instance_extensions = {};
	vec<c_str> device_extensions = {};

	StagingPool staging_pool = {};

private:
	vk::PhysicalDeviceFeatures physical_device_features;

protected:
	u64 messenger_warn_count = {};
	u64 messenger_err_count = {};

private:
	void load_default_textures();
};

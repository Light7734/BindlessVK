#pragma once

//
#include "Framework/Utils/Logger.hpp"
//

#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"
#include "BindlessVk/Shader/DescriptorSet.hpp"
#include "BindlessVk/Allocators/LayoutAllocator.hpp"
#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Material/MaterialSystem.hpp"
#include "BindlessVk/Model/ModelLoader.hpp"
#include "BindlessVk/Renderer/Renderer.hpp"
#include "BindlessVk/Shader/Shader.hpp"
#include "BindlessVk/Shader/ShaderLoader.hpp"
#include "BindlessVk/Texture/Texture.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Pools/StagingPool.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"

#include <GLFW/glfw3.h>

class Application
{
public:
	Application();
	virtual ~Application();

	virtual void on_tick(double delta_time) = 0;
	virtual void on_swapchain_recreate() = 0;

public:
	Logger logger = {};

	bvk::Instance instance = {};

	bvk::DebugUtils debug_utils = {};

	bvk::Gpu gpu = {};
	bvk::Surface::WindowSurface window_surface = {};
	bvk::Surface surface = {};
	bvk::Queues queues = {};
	bvk::Device device = {};
	bvk::Swapchain swapchain = {};

	bvk::VkContext vk_context = {};

	bvk::MemoryAllocator memory_allocator = {};
	bvk::LayoutAllocator layout_allocator = {};
	bvk::DescriptorAllocator descriptor_allocator = {};

	Scene scene = {};
	Window window = {};
	CameraController camera_controller = {};

	StagingPool staging_pool = {};

	bvk::Renderer renderer = {};
	scope<bvk::Rendergraph> render_graph = {};

	bvk::TextureLoader texture_loader = {};
	bvk::ModelLoader model_loader = {};
	bvk::ShaderLoader shader_loader = {};

	hash_map<u64, bvk::Model> models = {};
	hash_map<u64, bvk::Texture> textures = {};
	hash_map<u64, bvk::Shader> shaders = {};
	hash_map<u64, bvk::ShaderPipeline> shader_pipelines = {};
	hash_map<u64, bvk::ShaderPipeline::Configuration> shader_effect_configurations = {};
	hash_map<u64, bvk::Material> materials = {};

	vk::DescriptorPool descriptor_pool = {};

private:
	void create_window();
	void create_vk_context();
	void create_allocators();
	void create_descriptor_pool();
	void create_user_interface();
	void create_loaders();
	void create_renderer();

	void load_default_textures();

	auto get_instance_layers() const -> vec<c_str>;
	auto get_instance_extensions() const -> vec<c_str>;
	auto get_required_device_extensions() const -> vec<c_str>;
	auto get_required_physical_device_features() const -> vk::PhysicalDeviceFeatures;

	void destroy_user_interface();
};

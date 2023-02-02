#include "Framework/Core/Application.hpp"

#include <any>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cassert>
#include <imgui.h>
#include <utility>

Application::Application()
{
	create_window();
	create_vk_context();
	create_descriptor_pool();

	create_renderer();
	create_render_graph();

	create_user_interface();

	camera_controller = CameraController(&scene, &window);
	staging_pool = StagingPool(3, (1024u * 1024u * 256u), vk_context);

	create_loaders();
	load_default_textures();
}

Application::~Application()
{
	vk_context->get_device().waitIdle();

	destroy_models();
	destroy_user_interface();

	// @todo: fix this by making a class that encapsulates descriptor_pool and keep it alive using
	// smart pointers :)
	render_graph.reset();
	destroy_descriptor_pool();
}

void Application::create_window()
{
	using WindowHints = vec<pair<int, int>>;

	window.init(
	  WindowSpecs {
	    "BindlessVk",
	    1920u,
	    1080u,
	  },
	  WindowHints {
	    { GLFW_CLIENT_API, GLFW_NO_API },
	    { GLFW_FLOATING, GLFW_TRUE },
	  }
	);
}

void Application::create_vk_context()
{
	auto const layers = get_layers();
	auto const instance_extensions = get_instance_extensions();
	auto const physical_device_features = get_physical_device_features();
	auto const device_extensions = get_device_extensions();

	vk_context = std::make_shared<bvk::VkContext>(
	  layers,
	  instance_extensions,
	  device_extensions,
	  physical_device_features,

	  [&](vk::Instance instance) { return window.create_surface(instance); },
	  [&]() { return window.get_framebuffer_size(); },
	  [](vec<bvk::Gpu> const gpus) {
		  for (auto const &gpu : gpus) {
			  if (gpu.get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				  return gpu;
		  }

		  return gpus[0];
	  },

	  vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,

	  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
	    | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
	    | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
	    | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,

	  pair<fn<void(bvk::DebugCallbackSource, bvk::LogLvl, const str &, std::any)>, std::any> {
	    &Logger::bindlessvk_callback,
	    std::make_any<Logger const *const>(&logger),
	  }
	);
}
void Application::create_descriptor_pool()
{
	auto const device = vk_context->get_device();

	auto const pool_sizes = vec<vk::DescriptorPoolSize> {
		{ vk::DescriptorType::eSampler, 8000 },
		{ vk::DescriptorType::eCombinedImageSampler, 8000 },
		{ vk::DescriptorType::eSampledImage, 8000 },
		{ vk::DescriptorType::eStorageImage, 8000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 8000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 8000 },
		{ vk::DescriptorType::eUniformBuffer, 8000 },
		{ vk::DescriptorType::eStorageBuffer, 8000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 8000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 8000 },
		{ vk::DescriptorType::eInputAttachment, 8000 },
	};

	descriptor_pool = device.createDescriptorPool(vk::DescriptorPoolCreateInfo {
	  vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
	    | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	  100,
	  pool_sizes,
	});
}

void Application::create_renderer()
{
	renderer = std::make_unique<bvk::Renderer>(vk_context);
}

void Application::create_render_graph()
{
	render_graph = std::make_unique<bvk::RenderGraph>(
	  vk_context.get(),
	  descriptor_pool,
	  renderer->get_swapchain_images(),
	  renderer->get_swapchain_image_views()
	);
}

void Application::create_user_interface()
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.get_glfw_handle(), true);

	ImGui_ImplVulkan_InitInfo imgui_info {
		vk_context->get_instance(),
		vk_context->get_gpu(),
		vk_context->get_device(),
		vk_context->get_queues().graphics_index,
		vk_context->get_queues().graphics,
		{},
		descriptor_pool,
		{},
		true,
		static_cast<VkFormat>(vk_context->get_surface().color_format),
		BVK_MAX_FRAMES_IN_FLIGHT,
		BVK_MAX_FRAMES_IN_FLIGHT,
		static_cast<VkSampleCountFlagBits>(vk_context->get_gpu().get_max_color_and_depth_samples()),
	};


	assert_true(
	  ImGui_ImplVulkan_LoadFunctions(
	    [](c_str proc_name, void *data) {
		    auto const vk_context = reinterpret_cast<bvk::VkContext *>(data);
		    return vk_context->get_instance_proc_addr(proc_name);
	    },
	    (void *)vk_context.get()
	  ),
	  "ImGui failed to load vulkan functions"
	);

	ImGui_ImplVulkan_Init(&imgui_info, VK_NULL_HANDLE);

	vk_context->immediate_submit([](vk::CommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Application::create_loaders()
{
	texture_loader = bvk::TextureLoader(vk_context);
	model_loader = bvk::ModelLoader(vk_context);
	shader_loader = bvk::ShaderLoader(vk_context);
}

void Application::load_default_textures()
{
	u8 defaultTexturePixelData[] = { 255, 0, 255, 255 };
	textures[hash_str("default_2d")] = texture_loader.load_from_binary(
	  "default_2d",
	  defaultTexturePixelData,
	  1,
	  1,
	  sizeof(defaultTexturePixelData),
	  bvk::Texture::Type::e2D,
	  staging_pool.get_by_index(0)
	);

	textures[hash_str("default_cube")] = texture_loader.load_from_ktx(
	  "default_cube",
	  "Assets/cubemap_yokohama_rgba.ktx",
	  bvk::Texture::Type::eCubeMap,
	  staging_pool.get_by_index(0)
	);
}

auto Application::get_layers() const -> vec<c_str>
{
	return vec<c_str> {
		"VK_LAYER_KHRONOS_validation",
	};
}

auto Application::get_instance_extensions() const -> vec<c_str>
{
	auto instance_extensions = window.get_required_extensions();
	instance_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return instance_extensions;
}

auto Application::get_device_extensions() const -> vec<c_str>
{
	return vec<c_str> {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	};
}

auto Application::get_physical_device_features() const -> vk::PhysicalDeviceFeatures
{
	return vk::PhysicalDeviceFeatures {
		VK_FALSE, // robustBufferAccess
		VK_FALSE, // fullDrawIndexUint32
		VK_FALSE, // imageCubeArray
		VK_FALSE, // independentBlend
		VK_FALSE, // geometryShader
		VK_FALSE, // tessellationShader
		VK_FALSE, // sampleRateShading
		VK_FALSE, // dualSrcBlend
		VK_FALSE, // logicOp
		VK_TRUE,  // multiDrawIndirect
		VK_TRUE,  // drawIndirectFirstInstance
		VK_FALSE, // depthClamp
		VK_FALSE, // depthBiasClamp
		VK_FALSE, // fillModeNonSolid
		VK_FALSE, // depthBounds
		VK_FALSE, // wideLines
		VK_FALSE, // largePoints
		VK_FALSE, // alphaToOne
		VK_FALSE, // multiViewport
		VK_TRUE,  // samplerAnisotropy
		VK_FALSE, // textureCompressionETC2
		VK_FALSE, // textureCompressionASTC
		VK_FALSE, // textureCompressionBC
		VK_FALSE, // occlusionQueryPrecise
		VK_FALSE, // pipelineStatisticsQuery
		VK_FALSE, // vertexPipelineStoresAndAtomics
		VK_FALSE, // fragmentStoresAndAtomics
		VK_FALSE, // shaderTessellationAndGeometryPointSize
		VK_FALSE, // shaderImageGatherExtended
		VK_FALSE, // shaderStorageImageExtendedFormats
		VK_FALSE, // shaderStorageImageMultisample
		VK_FALSE, // shaderStorageImageReadWithoutFormat
		VK_FALSE, // shaderStorageImageWriteWithoutFormat
		VK_FALSE, // shaderUniformBufferArrayDynamicIndexing
		VK_FALSE, // shaderSampledImageArrayDynamicIndexing
		VK_FALSE, // shaderStorageBufferArrayDynamicIndexing
		VK_FALSE, // shaderStorageImageArrayDynamicIndexing
		VK_FALSE, // shaderClipDistance
		VK_FALSE, // shaderCullDistance
		VK_FALSE, // shaderFloat64
		VK_FALSE, // shaderInt64
		VK_FALSE, // shaderInt16
		VK_FALSE, // shaderResourceResidency
		VK_FALSE, // shaderResourceMinLod
		VK_FALSE, // sparseBinding
		VK_FALSE, // sparseResidencyBuffer
		VK_FALSE, // sparseResidencyImage2D
		VK_FALSE, // sparseResidencyImage3D
		VK_FALSE, // sparseResidency2Samples
		VK_FALSE, // sparseResidency4Samples
		VK_FALSE, // sparseResidency8Samples
		VK_FALSE, // sparseResidency16Samples
		VK_FALSE, // sparseResidencyAliased
		VK_FALSE, // variableMultisampleRate
		VK_FALSE, // inheritedQueries
	};
}

void Application::destroy_models()
{
	for (auto &[key, val] : models) {
		delete val.vertex_buffer;
		delete val.index_buffer;
	}

	models.clear();
}

void Application::destroy_user_interface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Application::destroy_descriptor_pool()
{
	auto const device = vk_context->get_device();

	device.resetDescriptorPool(descriptor_pool);
	device.destroyDescriptorPool(descriptor_pool);
}

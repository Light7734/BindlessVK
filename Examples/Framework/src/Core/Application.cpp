#include "Framework/Core/Application.hpp"

#include "Framework/UserInterface/ImguiVulkanBackend.hpp"

#include <any>
#include <backends/imgui_impl_glfw.h>
#include <cassert>
#include <imgui.h>
#include <utility>

Application::Application()
{
	create_window();
	create_vk_context();
	create_descriptor_pool();

	renderer = std::make_unique<bvk::Renderer>(vk_context);

	create_user_interface();

	camera_controller = CameraController(&scene, &window);
	staging_pool = StagingPool(3, (1024u * 1024u * 256u), vk_context);

	create_loaders();
	load_default_textures();
}

Application::~Application()
{
	auto const device = vk_context->get_device();

	device.waitIdle();

	models.clear();
	textures.clear();

	destroy_user_interface();

	device.destroyDescriptorPool(descriptor_pool);
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
	auto const physical_device_features = get_physical_device_features();
	auto const device_extensions = get_device_extensions();

	instance = std::make_unique<bvk::Instance>(get_instance_extensions(), get_layers());

	vk_context = std::make_shared<bvk::VkContext>(
	    instance.get(),
	    device_extensions,
	    physical_device_features,

	    [&](vk::Instance instance) { return window.create_surface(instance); },
	    [&]() { return window.get_framebuffer_size(); },
	    [](vec<bvk::Gpu> const gpus) {
		    for (auto const &gpu : gpus)
			    if (gpu.get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				    return gpu;

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
	    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
	    100,
	    pool_sizes,
	});
}

void Application::create_user_interface()
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.get_glfw_handle(), true);

	auto imgui_info = ImGui_ImplVulkan_InitInfo {
		*instance.get(),
		vk_context->get_gpu(),
		vk_context->get_device(),
		vk_context->get_queues().get_graphics_index(),
		vk_context->get_queues().get_graphics(),
		{},
		descriptor_pool,
		static_cast<VkFormat>(vk_context->get_surface().get_color_format()),
		BVK_MAX_FRAMES_IN_FLIGHT,
		BVK_MAX_FRAMES_IN_FLIGHT,
		static_cast<VkSampleCountFlagBits>(vk_context->get_gpu().get_max_color_and_depth_samples()),
	};

	assert_true(
	    ImGui_ImplVulkan_LoadFunctions(
	        [](c_str proc_name, void *data) {
		        auto const instance = reinterpret_cast<bvk::Instance *>(data);
		        return instance->get_proc_addr(proc_name);
	        },
	        (void *)instance.get()
	    ),
	    "ImGui failed to load vulkan functions"
	);

	ImGui_ImplVulkan_Init(&imgui_info);

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
	textures.emplace(
	    hash_str("default_texture"),
	    texture_loader.load_from_binary(
	        defaultTexturePixelData,
	        1,
	        1,
	        sizeof(defaultTexturePixelData),
	        bvk::Texture::Type::e2D,
	        staging_pool.get_by_index(0),
	        vk::ImageLayout::eShaderReadOnlyOptimal,
	        "default_texture"
	    )
	);

	textures.emplace(
	    hash_str("default_texture_cube"),
	    texture_loader.load_from_ktx(
	        "Assets/cubemap_yokohama_rgba.ktx",
	        bvk::Texture::Type::eCubeMap,
	        staging_pool.get_by_index(0),
	        vk::ImageLayout::eShaderReadOnlyOptimal,
	        "default_texture_cube"
	    )
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
	auto physical_device_features = vk::PhysicalDeviceFeatures {};

	physical_device_features.samplerAnisotropy = true;
	physical_device_features.multiDrawIndirect = true;
	physical_device_features.drawIndirectFirstInstance = true;

	return physical_device_features;
}

void Application::destroy_user_interface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

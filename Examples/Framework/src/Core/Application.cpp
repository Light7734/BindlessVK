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
	create_allocators();

	renderer = bvk::Renderer { &vk_context, &memory_allocator };
	create_descriptor_pool();

	create_user_interface();

	camera_controller = { &scene, &window };
	staging_pool = {
		3,
		1024u * 1024u * 256u,
		&vk_context,
		&memory_allocator,
	};

	create_loaders();
	load_default_textures();

	vertex_buffer = bvk::VertexBuffer {
		&vk_context,
		&memory_allocator,
		1024 * 1024 * 8,
	};
}

Application::~Application()
{
	device.vk().waitIdle();

	models.clear();
	textures.clear();

	destroy_user_interface();

	device.vk().destroyDescriptorPool(descriptor_pool);
}

void Application::create_window()
{
	window = Window(
	    Window::Specs {
	        "BindlessVk",
	        { 1920, 1080 },
	    },

	    Window::Hints {
	        { GLFW_CLIENT_API, GLFW_NO_API },
	        { GLFW_FLOATING, GLFW_TRUE },
	    }
	);
}

void Application::create_vk_context()
{
	instance = {
		{
		    get_instance_extensions(),
		    get_instance_layers(),
		},
	};

	debug_utils = {
		&instance,

		{
		    &Logger::bindlessvk_callback,
		    std::make_any<Logger const *const>(&logger),
		},

		{
		    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,

		    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		        | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
		},
	};

	window_surface = window.create_vulkan_surface(instance.vk());

	gpu = bvk::Gpu::pick_by_score(
	    &instance,
	    window_surface.surface,

	    {
	        get_required_physical_device_features(),
	        get_required_device_extensions(),
	    },

	    [](auto gpu) {
		    auto const properties = gpu.vk().getProperties();

		    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			    return (u32)100'000 + properties.limits.maxImageDimension2D;

		    return properties.limits.maxImageDimension2D;
	    }
	);

	surface = {
		&window_surface,
		&gpu,

		[this]() { return window.get_framebuffer_size(); },

		[](vk::PresentModeKHR present_mode) {
		    if (present_mode == vk::PresentModeKHR::eFifo)
			    return 10'000;

		    return 0;
		},

		[](vk::SurfaceFormatKHR format) {
		    if (format.format == vk::Format::eB8G8R8A8Srgb
		        && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			    return 10'000;

		    return 0;
		},
	};

	device = { &gpu };

	queues = { &device, &gpu };

	vk_context = { &instance, &debug_utils, &surface, &gpu, &queues, &device };
}

void Application::create_allocators()
{
	memory_allocator = { &vk_context };
	layout_allocator = { &vk_context };
	descriptor_allocator = { &vk_context };
}

void Application::create_descriptor_pool()
{
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

	descriptor_pool = device.vk().createDescriptorPool({
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
		instance.vk(),
		gpu.vk(),
		device.vk(),
		queues.get_graphics_index(),
		queues.get_graphics(),
		{},
		descriptor_pool,
		static_cast<VkFormat>(surface.get_color_format()),
		bvk::max_frames_in_flight,
		bvk::max_frames_in_flight,
		static_cast<VkSampleCountFlagBits>(gpu.get_max_color_and_depth_samples()),
	};

	assert_true(
	    ImGui_ImplVulkan_LoadFunctions(
	        [](c_str proc_name, void *data) {
		        auto const instance = reinterpret_cast<bvk::Instance *>(data);
		        return instance->get_proc_addr(proc_name);
	        },
	        reinterpret_cast<void *>(&instance)
	    ),
	    "ImGui failed to load vulkan functions"
	);

	ImGui_ImplVulkan_Init(&imgui_info);

	device.immediate_submit([](vk::CommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); }
	);

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Application::create_loaders()
{
	texture_loader = { &vk_context, &memory_allocator };
	model_loader = { &vk_context, &memory_allocator };
	shader_loader = { &vk_context };
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

auto Application::get_instance_layers() const -> vec<c_str>
{
	return {
		"VK_LAYER_KHRONOS_validation",
	};
}

auto Application::get_instance_extensions() const -> vec<c_str>
{
	auto instance_extensions = window.get_required_extensions();
	instance_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return instance_extensions;
}

auto Application::get_required_device_extensions() const -> vec<c_str>
{
	return {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	};
}

auto Application::get_required_physical_device_features() const -> vk::PhysicalDeviceFeatures
{
	auto physical_device_features = vk::PhysicalDeviceFeatures {};

	physical_device_features.geometryShader = true;
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

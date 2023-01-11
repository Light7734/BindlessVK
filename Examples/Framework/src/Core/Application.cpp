#include "Framework/Core/Application.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <utility>

// @todo: refactor this out
static void initialize_imgui(bvk::Device* device, bvk::Renderer& renderer, Window& window)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.get_glfw_handle(), true);

	ImGui_ImplVulkan_InitInfo initInfo {
		.Instance              = device->instance,
		.PhysicalDevice        = device->physical,
		.Device                = device->logical,
		.Queue                 = device->graphics_queue,
		.DescriptorPool        = renderer.get_descriptor_pool(),
		.UseDynamicRendering   = true,
		.ColorAttachmentFormat = static_cast<VkFormat>(device->surface_format.format),
		.MinImageCount         = BVK_MAX_FRAMES_IN_FLIGHT,
		.ImageCount            = BVK_MAX_FRAMES_IN_FLIGHT,
		.MSAASamples           = static_cast<VkSampleCountFlagBits>(device->max_samples),
	};
	std::pair userData = std::make_pair(device->get_vk_instance_proc_addr_func, device->instance);

	ASSERT(
	  ImGui_ImplVulkan_LoadFunctions(
	    [](const char* func, void* data) {
		    auto [vkGetProcAddr, instance] = *(std::pair<PFN_vkGetInstanceProcAddr, vk::Instance>*)data;
		    return vkGetProcAddr(instance, func);
	    },
	    (void*)&userData
	  ),
	  "ImGui failed to load vulkan functions"
	);

	ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

	device->immediate_submit([](vk::CommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

static void destroy_imgui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

Application::Application()
    : instance_extensions {
	    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    }
    , device_extensions {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    }
{
	window.init(
	  WindowSpecs {
	    .title  = "BindlessVk",
	    .width  = 1920u,
	    .height = 1080u,
	  },
	  {
	    { GLFW_CLIENT_API, GLFW_NO_API },
	    { GLFW_FLOATING, GLFW_TRUE },
	  }
	);

	std::vector<const char*> required_surface_extensions = window.get_required_extensions();

	instance_extensions.insert(
	  instance_extensions.end(),
	  required_surface_extensions.begin(),
	  required_surface_extensions.end()
	);


	device_system.init(
	  { "VK_LAYER_KHRONOS_validation" },
	  instance_extensions,
	  device_extensions,

	  [&](vk::Instance instance) { return window.create_surface(instance); },
	  [&]() { return window.get_framebuffer_size(); },

	  true,
	  vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
	    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,

	  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
	    | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
	    | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,

	  &vulkan_debug_message_callback,
	  this
	);

	bvk::Device* device = device_system.get_device();

	staging_pool = StagingPool(2, (1024u * 1024u * 256u), device);

	texture_system.init(device);

	uint8_t defaultTexturePixelData[4] = { 255, 0, 255, 255 };
	texture_system.create_from_buffer(
	  "default",
	  defaultTexturePixelData,
	  1,
	  1,
	  sizeof(defaultTexturePixelData),
	  bvk::Texture::Type::e2D
	);

	texture_system.create_from_ktx(
	  "default_cube",
	  "Assets/cubemap_yokohama_rgba.ktx",
	  bvk::Texture::Type::eCubeMap
	);

	material_system.init(device);

	renderer     = bvk::Renderer(device);
	model_loader = bvk::ModelLoader(device, &texture_system);

	initialize_imgui(device, renderer, window);

	camera_controller = CameraController(&scene, &window);
}

Application::~Application()
{
	device_system.get_device()->logical.waitIdle();
	destroy_imgui();

	texture_system.reset();
	material_system.reset();

	for (auto& [key, val] : models) {
		delete val.vertex_buffer;
		delete val.index_buffer;
	}

	models.clear();

	staging_pool.destroy_buffers();

	device_system.reset();
}

VkBool32 Application::vulkan_debug_message_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_types,
  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
  void* user_data
)
{
	Application* application = (Application*)user_data;

	std::string type = message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ?
	                     "GENERAL" :
	                   message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	                       && message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ?
	                     "VALIDATION | PERFORMANCE" :
	                   message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ?
	                     "VALIDATION" :
	                     "PERFORMANCE";

	std::string message             = (callback_data->pMessage);
	std::string url                 = {};
	std::string vulkanSpecStatement = {};

	// Remove beginning sections ( we'll log them ourselves )
	auto pos = message.find_last_of("|");
	if (pos != std::string::npos) {
		message = message.substr(pos + 2);
	}

	// Separate url section of message
	pos = message.find_last_of("(");
	if (pos != std::string::npos) {
		url     = message.substr(pos + 1, message.length() - (pos + 2));
		message = message.substr(0, pos);
	}

	// Separate the "Vulkan spec states:" section of the message
	pos = message.find("The Vulkan spec states:");
	if (pos != std::string::npos) {
		size_t len          = std::strlen("The Vulkan spec states: ");
		vulkanSpecStatement = message.substr(pos + len, message.length() - pos - len);
		message             = message.substr(0, pos);
	}

	if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		application->messenger_err_count++;

		LOG(err, "[ __VULKAN_MESSAGE_CALLBACK__ ]");
		LOG(err, "    Type -> {} - {}", type, callback_data->pMessageIdName);
		LOG(err, "    Url  -> {}", url);
		LOG(err, "    Msg  -> {}", message);
		LOG(err, "    Spec -> {}", vulkanSpecStatement);

		if (callback_data->objectCount) {
			LOG(err, "    {} OBJECTS:", callback_data->objectCount);
			for (uint32_t object = 0; object < callback_data->objectCount; object++) {
				LOG(
				  err,
				  "            [{}] {} -> Addr: {}, Name: {}",
				  object,
				  string_VkObjectType(callback_data->pObjects[object].objectType)
				    + std::strlen("VK_OBJECT_TYPE_"),
				  fmt::ptr((void*)callback_data->pObjects[object].objectHandle),
				  callback_data->pObjects[object].pObjectName ?
				    callback_data->pObjects[object].pObjectName :
				    "(Undefined)"
				);
			}
		}

		if (callback_data->cmdBufLabelCount) {
			LOG(err, "    {} COMMAND BUFFER LABELS:", callback_data->cmdBufLabelCount);
			for (uint32_t label = 0; label < callback_data->cmdBufLabelCount; label++) {
				LOG(
				  err,
				  "            [{}]-> {} ({}, {}, {}, {})",
				  label,
				  callback_data->pCmdBufLabels[label].pLabelName,
				  callback_data->pCmdBufLabels[label].color[0],
				  callback_data->pCmdBufLabels[label].color[1],
				  callback_data->pCmdBufLabels[label].color[2],
				  callback_data->pCmdBufLabels[label].color[3]
				);
			}
		}

		LOG(err, "");
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		application->messenger_warn_count++;
		LOG(warn, "{} - {}-> {}", type, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		LOG(info, "{} - {}-> {}", type, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else {
		LOG(trace, "{} - {}-> {}", type, callback_data->pMessageIdName, callback_data->pMessage);
	}


	return static_cast<VkBool32>(VK_FALSE);
}

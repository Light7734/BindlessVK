#include "Framework/Core/Application.hpp"

#include <any>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cassert>
#include <imgui.h>
#include <utility>

VkBool32 Application::vulkan_debug_message_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_types,
  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
  void* user_data
)
{
	using namespace spdlog::level;

	Application* app = (Application*)user_data;

	std::string type = message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ?
	                     "GENERAL" :
	                   message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	                       && message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ?
	                     "VALIDATION | PERFORMANCE" :
	                   message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ?
	                     "VALIDATION" :
	                     "PERFORMANCE";

	std::string message = (callback_data->pMessage);
	std::string url = {};
	std::string vulkanSpecStatement = {};

	// Remove beginning sections ( we'll log them ourselves )
	auto pos = message.find_last_of("|");
	if (pos != std::string::npos) {
		message = message.substr(pos + 2);
	}

	// Separate url section of message
	pos = message.find_last_of("(");
	if (pos != std::string::npos) {
		url = message.substr(pos + 1, message.length() - (pos + 2));
		message = message.substr(0, pos);
	}

	// Separate the "Vulkan spec states:" section of the message
	pos = message.find("The Vulkan spec states:");
	if (pos != std::string::npos) {
		size_t len = std::strlen("The Vulkan spec states: ");
		vulkanSpecStatement = message.substr(pos + len, message.length() - pos - len);
		message = message.substr(0, pos);
	}

	if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		app->messenger_err_count++;

		app->logger.log(err, "[VK]:");
		app->logger.log(err, "    Type -> {} - {}", type, callback_data->pMessageIdName);
		app->logger.log(err, "    Url  -> {}", url);
		app->logger.log(err, "    Msg  -> {}", message);


		if (callback_data->objectCount) {
			app->logger.log(err, "    {} OBJECTS:", callback_data->objectCount);
			for (uint32_t object = 0; object < callback_data->objectCount; object++) {
				app->logger.log(
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
			app->logger.log(err, "    {} COMMAND BUFFER LABELS:", callback_data->cmdBufLabelCount);
			for (uint32_t label = 0; label < callback_data->cmdBufLabelCount; label++) {
				app->logger.log(
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

		app->logger.log(err, "");
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		app->messenger_warn_count++;
		app->logger.log(
		  warn,
		  "[VK]: {} - {}-> {}",
		  type,
		  callback_data->pMessageIdName,
		  callback_data->pMessage
		);
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		app->logger.log(
		  info,
		  "[VK]: {} - {}-> {}",
		  type,
		  callback_data->pMessageIdName,
		  callback_data->pMessage
		);
	}
	else {
		app->logger.log(
		  trace,
		  "[VK]: {} - {}-> {}",
		  type,
		  callback_data->pMessageIdName,
		  callback_data->pMessage
		);
	}


	return static_cast<VkBool32>(VK_FALSE);
}

/// Callback function called after successful vkAllocateMemory.
static void vma_allocate_device_memory_callback(
  VmaAllocator VMA_NOT_NULL allocator,
  uint32_t memory_type,
  VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
  VkDeviceSize size,
  void* VMA_NULLABLE user_data
)
{
	auto* app = reinterpret_cast<Application*>(user_data);
	app->logger.log(spdlog::level::trace, "[VMA]: Allocate device memory -> {}", size);
}

/// Callback function called before vkFreeMemory.
static void vma_free_device_memory_callback(
  VmaAllocator VMA_NOT_NULL allocator,
  uint32_t memory_type,
  VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
  VkDeviceSize size,
  void* VMA_NULLABLE user_data
)
{
	auto* app = reinterpret_cast<Application*>(user_data);
	app->logger.log(spdlog::level::trace, "[VMA]: Free device memory -> {}", size);
}

static void bindlessvk_debug_callback(bvk::LogLvl severity, const str& message, std::any user_data)
{
	auto* logger = std::any_cast<Logger*>(user_data);
	logger->log((spdlog::level::level_enum)(int)severity, "[BVK]: {}", message);
}

// @todo: refactor this out
static void initialize_imgui(bvk::VkContext* vk_context, bvk::Renderer& renderer, Window& window)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.get_glfw_handle(), true);

	ImGui_ImplVulkan_InitInfo initInfo {
		.Instance = vk_context->get_instance(),
		.PhysicalDevice = vk_context->get_gpu(),
		.Device = vk_context->get_device(),
		.Queue = vk_context->get_queues().graphics,
		.DescriptorPool = renderer.get_descriptor_pool(),
		.UseDynamicRendering = true,
		.ColorAttachmentFormat = static_cast<VkFormat>(vk_context->get_surface().color_format),
		.MinImageCount = BVK_MAX_FRAMES_IN_FLIGHT,
		.ImageCount = BVK_MAX_FRAMES_IN_FLIGHT,
		.MSAASamples = static_cast<VkSampleCountFlagBits>(vk_context->get_max_color_and_depth_samples()
		),
	};
	auto userData = std::make_pair(vk_context->get_vk_instance_proc_addr, vk_context->get_instance());

	assert_true(
	  ImGui_ImplVulkan_LoadFunctions(
	    [](c_str func, void* data) {
		    auto [vkGetProcAddr, instance] = *(pair<PFN_vkGetInstanceProcAddr, vk::Instance>*)data;
		    return vkGetProcAddr(instance, func);
	    },
	    (void*)&userData
	  ),
	  "ImGui failed to load vulkan functions"
	);

	ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

	vk_context->immediate_submit([](vk::CommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

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
	    .title = "BindlessVk",
	    .width = 1920u,
	    .height = 1080u,
	  },
	  {
	    { GLFW_CLIENT_API, GLFW_NO_API },
	    { GLFW_FLOATING, GLFW_TRUE },
	  }
	);

	vec<c_str> required_surface_extensions = window.get_required_extensions();

	instance_extensions.insert(
	  instance_extensions.end(),
	  required_surface_extensions.begin(),
	  required_surface_extensions.end()
	);

	vk_context = bvk::VkContext(
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
	  this,

	  &vma_allocate_device_memory_callback,
	  &vma_free_device_memory_callback,
	  this,

	  &bindlessvk_debug_callback,
	  std::make_any<Logger*>(&logger)
	);

	const auto device = vk_context.get_device();

	staging_pool = StagingPool(3, (1024u * 1024u * 256u), &vk_context);

	texture_loader = bvk::TextureLoader(&vk_context);
	model_loader = bvk::ModelLoader(&vk_context, &texture_loader);
	shader_loader = bvk::ShaderLoader(&vk_context);

	renderer = bvk::Renderer(&vk_context);

	initialize_imgui(&vk_context, renderer, window);

	camera_controller = CameraController(&scene, &window);

	u8 defaultTexturePixelData[4] = { 255, 0, 255, 255 };
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

	const auto pool_sizes = vec<vk::DescriptorPoolSize> {
		{ vk::DescriptorType::eSampler, 1000u },
		{ vk::DescriptorType::eCombinedImageSampler, 1000u },
		{ vk::DescriptorType::eSampledImage, 1000u },
		{ vk::DescriptorType::eStorageImage, 1000u },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000u },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000u },
		{ vk::DescriptorType::eUniformBuffer, 1000u },
		{ vk::DescriptorType::eStorageBuffer, 1000u },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000u },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000u },
		{ vk::DescriptorType::eInputAttachment, 1000u },
	};

	descriptor_pool = device.createDescriptorPool(
	  vk::DescriptorPoolCreateInfo {
	    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    100u,
	    static_cast<u32>(pool_sizes.size()),
	    pool_sizes.data(),
	  },
	  nullptr
	);
}

Application::~Application()
{
	vk_context.get_device().waitIdle();
	destroy_imgui();

	for (auto& [key, val] : models) {
		delete val.vertex_buffer;
		delete val.index_buffer;
	}

	models.clear();
}

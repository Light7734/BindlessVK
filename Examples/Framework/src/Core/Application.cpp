#include "Framework/Core/Application.hpp"

#include <any>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cassert>
#include <imgui.h>
#include <utility>

static void bindlessvk_debug_callback(
  bvk::DebugCallbackSource const source,
  bvk::LogLvl const severity,
  str const &message,
  std::any const user_data
)
{
	try {
		auto const *const logger = std::any_cast<Logger const *const>(user_data);

		auto const source_str = //
		  source == bvk::DebugCallbackSource::eBindlessVk       ? "BindlessVk" :
		  source == bvk::DebugCallbackSource::eValidationLayers ? "Validation Layers" :
		                                                          "Memory Allocator";


		logger->log((spdlog::level::level_enum)(int)severity, "[{}]: {}", source_str, message);
	}

	catch (std::bad_any_cast bad_cast_exception) {
		std::cout << " we fucked up big time " << std::endl;
	}
}

// @todo: refactor this out
static void initialize_imgui(bvk::VkContext *vk_context, bvk::Renderer &renderer, Window &window)
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

	auto pfn_get_vk_instance_proc_addr = vk_context->get_pfn_get_vk_instance_proc_addr();
	auto userData = std::make_pair(pfn_get_vk_instance_proc_addr, vk_context->get_instance());

	assert_true(
	  ImGui_ImplVulkan_LoadFunctions(
	    [](c_str func, void *data) {
		    auto [vkGetProcAddr, instance] = *(pair<PFN_vkGetInstanceProcAddr, vk::Instance> *)data;
		    return vkGetProcAddr(instance, func);
	    },
	    (void *)&userData
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

	physical_device_features = vk::PhysicalDeviceFeatures {
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
	  physical_device_features,

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

	  &bindlessvk_debug_callback,
	  std::make_any<Logger const *const>(&logger)
	);

	texture_loader = bvk::TextureLoader(&vk_context);
	model_loader = bvk::ModelLoader(&vk_context, &texture_loader);
	shader_loader = bvk::ShaderLoader(&vk_context);

	renderer = bvk::Renderer(&vk_context);

	initialize_imgui(&vk_context, renderer, window);

	camera_controller = CameraController(&scene, &window);

	staging_pool = StagingPool(3, (1024u * 1024u * 256u), &vk_context);
	load_default_textures();

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

	descriptor_pool = vk_context.get_device().createDescriptorPool(
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

	for (auto &[key, val] : models) {
		delete val.vertex_buffer;
		delete val.index_buffer;
	}

	models.clear();
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

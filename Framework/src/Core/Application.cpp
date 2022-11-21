#include "Framework/Core/Application.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

// @todo: refactor this out
static void InitializeImgui(bvk::Device* device, bvk::Renderer& renderer, Window& window)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.GetGlfwHandle(), true);

	ImGui_ImplVulkan_InitInfo initInfo {
		.Instance              = device->instance,
		.PhysicalDevice        = device->physical,
		.Device                = device->logical,
		.Queue                 = device->graphicsQueue,
		.DescriptorPool        = renderer.GetDescriptorPool(),
		.UseDynamicRendering   = true,
		.ColorAttachmentFormat = static_cast<VkFormat>(device->surfaceFormat.format),
		.MinImageCount         = MAX_FRAMES_IN_FLIGHT,
		.ImageCount            = MAX_FRAMES_IN_FLIGHT,
		.MSAASamples           = static_cast<VkSampleCountFlagBits>(device->maxDepthColorSamples),
	};
	std::pair userData = std::make_pair(device->vkGetInstanceProcAddr, device->instance);

	ASSERT(ImGui_ImplVulkan_LoadFunctions(
	           [](const char* func, void* data) {
		           auto [vkGetProcAddr, instance] = *(std::pair<PFN_vkGetInstanceProcAddr, vk::Instance>*)data;
		           return vkGetProcAddr(instance, func);
	           },
	           (void*)&userData),
	       "ImGui failed to load vulkan functions");

	ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

	renderer.ImmediateSubmit([](vk::CommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

static void DestroyImgui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}


Application::Application()
    : m_InstanceExtensions {
	    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    }
    , m_DeviceExtensions {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    }
{
	m_Window.Init({
	    .specs = {
	        .title  = "BindlessVk",
	        .width  = 1920u,
	        .height = 1080u,
	    },
	    .hints = {
	        { GLFW_CLIENT_API, GLFW_NO_API },
	        { GLFW_FLOATING, GLFW_TRUE },
	    },
	});

	m_DeviceSystem.Init({
	    .layers             = { "VK_LAYER_KHRONOS_validation" },
	    .instanceExtensions = m_InstanceExtensions,
	    .deviceExtensions   = m_DeviceExtensions,
	    .windowExtensions   = m_Window.GetRequiredExtensions(),

	    .createWindowSurfaceFunc = [&](vk::Instance instance) { return m_Window.CreateSurface(instance); },

	    .enableDebugging           = true,
	    .debugMessengerSeverities  = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	    .debugMessengerTypes       = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	    .debugMessengerCallback    = &VulkanDebugMessageCallback,
	    .debugMessengerUserPointer = this,
	});
	bvk::Device* device = m_DeviceSystem.GetDevice();

	m_TextureSystem.Init({ device });

	uint8_t defaultTexturePixelData[4] = { 255, 0, 255, 255 };
	m_TextureSystem.CreateFromBuffer({
	    .name   = "default",
	    .pixels = defaultTexturePixelData,
	    .width  = 1,
	    .height = 1,
	    .size   = sizeof(defaultTexturePixelData),
	    .type   = bvk::Texture::Type::e2D,
	});

	m_TextureSystem.CreateFromKTX({
	    "default_cube",
	    "Assets/cubemap_yokohama_rgba.ktx",
	    bvk::Texture::Type::eCubeMap,
	});

	m_Renderer.Init({ device });

	m_MaterialSystem.Init({ device });

	// @todo: refactor out getting a command pool from renderer
	m_ModelSystem.Init({ device, m_Renderer.GetCommandPool() });


	InitializeImgui(device, m_Renderer, m_Window);


	m_CameraController = CameraController({ .scene = &m_Scene, .window = &m_Window });
}


Application::~Application()
{
	m_DeviceSystem.GetDevice()->logical.waitIdle();
	DestroyImgui();

	m_Renderer.Reset();
	m_TextureSystem.Reset();
	m_MaterialSystem.Reset();
	m_ModelSystem.Reset();
	m_DeviceSystem.Reset();
}

VkBool32 Application::VulkanDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	Application* application = (Application*)pUserData;

	std::string type = messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT                                                                       ? "GENERAL" :
	                   messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT && messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ? "VALIDATION | PERFORMANCE" :
	                   messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT                                                                    ? "VALIDATION" :
	                                                                                                                                                       "PERFORMANCE";

	std::string message             = (pCallbackData->pMessage);
	std::string url                 = {};
	std::string vulkanSpecStatement = {};

	// Remove beginning sections ( we'll log them ourselves )
	auto pos = message.find_last_of("|");
	if (pos != std::string::npos)
	{
		message = message.substr(pos + 2);
	}

	// Separate url section of message
	pos = message.find_last_of("(");
	if (pos != std::string::npos)
	{
		url     = message.substr(pos + 1, message.length() - (pos + 2));
		message = message.substr(0, pos);
	}

	// Separate the "Vulkan spec states:" section of the message
	pos = message.find("The Vulkan spec states:");
	if (pos != std::string::npos)
	{
		size_t len          = std::strlen("The Vulkan spec states: ");
		vulkanSpecStatement = message.substr(pos + len, message.length() - pos - len);
		message             = message.substr(0, pos);
	}

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		application->m_MessengerErrCount++;

		LOG(err, "[ __VULKAN_MESSAGE_CALLBACK__ ]");
		LOG(err, "    Type -> {} - {}", type, pCallbackData->pMessageIdName);
		LOG(err, "    Url  -> {}", url);
		LOG(err, "    Msg  -> {}", message);
		LOG(err, "    Spec -> {}", vulkanSpecStatement);

		if (pCallbackData->objectCount)
		{
			LOG(err, "    {} OBJECTS:", pCallbackData->objectCount);
			for (uint32_t object = 0; object < pCallbackData->objectCount; object++)
			{
				LOG(err, "            [{}] {} -> Addr: {}, Name: {}",
				    object,
				    string_VkObjectType(pCallbackData->pObjects[object].objectType) + std::strlen("VK_OBJECT_TYPE_"),
				    fmt::ptr((void*)pCallbackData->pObjects[object].objectHandle),
				    pCallbackData->pObjects[object].pObjectName ? pCallbackData->pObjects[object].pObjectName : "(Undefined)");
			}
		}

		if (pCallbackData->cmdBufLabelCount)
		{
			LOG(err, "    {} COMMAND BUFFER LABELS:", pCallbackData->cmdBufLabelCount);
			for (uint32_t label = 0; label < pCallbackData->cmdBufLabelCount; label++)
			{
				LOG(err, "            [{}]-> {} ({}, {}, {}, {})",
				    label,
				    pCallbackData->pCmdBufLabels[label].pLabelName,
				    pCallbackData->pCmdBufLabels[label].color[0],
				    pCallbackData->pCmdBufLabels[label].color[1],
				    pCallbackData->pCmdBufLabels[label].color[2],
				    pCallbackData->pCmdBufLabels[label].color[3]);
			}
		}

		LOG(err, "");
	}
	else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		application->m_MessengerWarnCount++;
		LOG(warn, "{} - {}-> {}", type, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		LOG(info, "{} - {}-> {}", type, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else
	{
		LOG(trace, "{} - {}-> {}", type, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}


	return static_cast<VkBool32>(VK_FALSE);
}

#include "Graphics/Device.h"

#include "Core/Window.h"

#include <GLFW/glfw3.h>

Device::Device(Window* window)
    : m_Window(window)
    , m_ValidationLayers { "VK_LAYER_KHRONOS_validation" }
    , m_RequiredExtensions { VK_EXT_DEBUG_UTILS_EXTENSION_NAME }
    , m_LogicalDeviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME }

{
    // init vulkan
    VKC(volkInitialize());

    FilterValidationLayers();
    FetchRequiredExtensions();
    auto debugMessageCreateInfo = SetupDebugMessageCallback();

    CreateVkInstance(debugMessageCreateInfo);
    CreateWindowSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
}

Device::~Device()
{
    // destroy device
    vkDestroyDevice(m_LogicalDevice, nullptr);
    vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
    vkDestroyInstance(m_VkInstance, nullptr);
}

void Device::CreateVkInstance(VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo)
{
    // application info
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

        .pApplicationName   = "VulkanRenderer",
        .applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),

        .pEngineName   = "",
        .engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),

        .apiVersion = VK_API_VERSION_1_2
    };

    // instance create-info
    VkInstanceCreateInfo instanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debugMessageCreateInfo,

        .pApplicationInfo = &appInfo,

        .enabledLayerCount   = static_cast<uint32_t>(m_ValidationLayers.size()),
        .ppEnabledLayerNames = m_ValidationLayers.data(),

        .enabledExtensionCount   = static_cast<uint32_t>(m_RequiredExtensions.size()),
        .ppEnabledExtensionNames = m_RequiredExtensions.data(),
    };

    // vulkan instance
    VKC(vkCreateInstance(&instanceCreateInfo, nullptr, &m_VkInstance));
    volkLoadInstance(m_VkInstance);
}

void Device::CreateWindowSurface()
{
    ASSERT(m_Window, "Invalid window pointer");

    glfwCreateWindowSurface(m_VkInstance, m_Window->GetHandle(), nullptr, &m_Surface);
}

void Device::PickPhysicalDevice()
{
    // fetch physical devices
    uint32_t deviceCount = 0u;
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
    ASSERT(deviceCount, "Failed to find a GPU with vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

    // select most suitable physical device
    uint8_t highestDeviceScore = 0u;
    for (const auto& device : devices)
    {
        uint32_t deviceScore = 0u;

        // fetch properties & features
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures   features;

        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        // geometry shader is needed for rendering
        if (!features.geometryShader)
            continue;

        // discrete gpu is favored
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            deviceScore += 10000u;

        deviceScore += properties.limits.maxImageDimension2D;

        // more suitable device found
        if (deviceScore > highestDeviceScore)
        {
            m_PhysicalDevice = device;

            // check if device supports required queue families
            FetchSupportedQueueFamilies();
            if (!m_QueueFamilyIndices)
                m_PhysicalDevice = VK_NULL_HANDLE;
            else
                highestDeviceScore = deviceScore;
        }
    }

    ASSERT(m_PhysicalDevice, "Failed to find suitable GPU for vulkan");
}

void Device::CreateLogicalDevice()
{
    // fetch properties & features
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures   deviceFeatures;

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

    // deviceFeatures.samplerAnisotropy = VK_TRUE;

    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    deviceQueueCreateInfos.reserve(m_QueueFamilyIndices.indices.size());

    // device queue create-infos
    float queuePriority = 1.0f;
    for (const auto& queueFamilyIndex : m_QueueFamilyIndices.indices)
    {
        VkDeviceQueueCreateInfo deviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,

            .queueFamilyIndex = queueFamilyIndex,
            .queueCount       = 1u,

            .pQueuePriorities = &queuePriority,
        };

        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    // device create-info
    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

        .queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
        .pQueueCreateInfos    = deviceQueueCreateInfos.data(),

        .enabledExtensionCount   = static_cast<uint32_t>(m_LogicalDeviceExtensions.size()),
        .ppEnabledExtensionNames = m_LogicalDeviceExtensions.data(),

        .pEnabledFeatures = &deviceFeatures,
    };

    // create logical device and get it's queues
    VKC(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice));
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.graphics.value(), 0u, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.present.value(), 0u, &m_PresentQueue);

    ASSERT(m_GraphicsQueue, "Pipeline::CreateLogicalDevice: failed to get graphics queue");
    ASSERT(m_PresentQueue, "Pipeline::CreateLogicalDevice: failed to get present queue");

    // validate device ...
    // fetch device extensions
    uint32_t deviceExtensionsCount = 0u;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, nullptr);

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionsCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, deviceExtensions.data());

    // check if device supports the required extensions
    std::set<std::string> requiredExtensions(m_LogicalDeviceExtensions.begin(), m_LogicalDeviceExtensions.end());

    for (const auto& deviceExtension : deviceExtensions)
        requiredExtensions.erase(std::string(deviceExtension.extensionName));

    if (!requiredExtensions.empty())
    {
        LOG(critical, "Pipeline::FetchDeviceExtensions: device does not supprot required extensions: ");

        for (auto requiredExtension : requiredExtensions)
            LOG(critical, "        {}", requiredExtension);

        ASSERT(false, "Pipeline::FetchDeviceExtensions: aforementioned device extensinos are not supported");
    }
}

void Device::FilterValidationLayers()
{
    // fetch available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // remove requested layers that are not supported
    for (const char* layerName : m_ValidationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        // layer not found
        if (!layerFound)
        {
            m_ValidationLayers.erase(std::find(m_ValidationLayers.begin(), m_ValidationLayers.end(), layerName));
            LOG(err, "Pipeline::FilterValidationLayers: failed to find validation layer: {}", layerName);
        }
    }
}

void Device::FetchRequiredExtensions()
{
    // fetch required global extensions
    uint32_t     glfwExtensionsCount = 0u;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    // append glfw's required extensions to our required extensions
    m_RequiredExtensions.insert(m_RequiredExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionsCount);
}

void Device::FetchSupportedQueueFamilies()
{
    m_QueueFamilyIndices = {};

    // fetch queue families
    uint32_t queueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    // find queue family indices
    uint32_t index = 0u;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_QueueFamilyIndices.graphics = index;
            LOG(info, "Pipeline::FetchSupportedQueueFamilies: graphics index: {}", index);
        }

        VkBool32 hasPresentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, m_Surface, &hasPresentSupport);

        if (hasPresentSupport)
        {
            m_QueueFamilyIndices.present = index;
            LOG(info, "Pipeline::FetchSupportedQueueFamilies: present index: {}", index);
        }

        index++;
    }

    m_QueueFamilyIndices.indices = { m_QueueFamilyIndices.graphics.value(), m_QueueFamilyIndices.present.value() };
}

VkDebugUtilsMessengerCreateInfoEXT Device::SetupDebugMessageCallback()
{
    // debug messenger create-info
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pUserData = nullptr
    };

    // debug message callback
    debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                  VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                  void*                                       pUserData) {
        LOGVk(trace, "{}", pCallbackData->pMessage);
        return static_cast<VkBool32>(VK_FALSE);
    };

    return debugMessengerCreateInfo;
}
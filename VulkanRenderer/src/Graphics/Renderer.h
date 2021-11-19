#pragma once

#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/RendererPrograms/QuadRendererProgram.h"

#include <glm/glm.hpp>

#include <volk.h>

struct GLFWwindow;
class Shader;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    std::vector<uint32_t> indices;

    inline bool IsSuitableForRendering() const { return graphics.has_value() && present.has_value(); }

    operator bool() const { return IsSuitableForRendering(); };
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;

    inline bool IsSuitableForRendering() const { !formats.empty() && !presentModes.empty(); }

    operator bool() const { return IsSuitableForRendering(); }
};

class Renderer
{
private:
    // window
    GLFWwindow*  m_WindowHandle;
    VkSurfaceKHR m_Surface;

    // device
    DeviceContext    m_DeviceContext;
    VkInstance       m_VkInstance;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice         m_LogicalDevice;

    // queue
    QueueFamilyIndices m_QueueFamilyIndices;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    // validation layers & extensions
    std::vector<const char*> m_ValidationLayers;
    std::vector<const char*> m_RequiredExtensions;
    std::vector<const char*> m_LogicalDeviceExtensions;

    // render pass
    VkRenderPass m_RenderPass;

    // swap chain
    VkSwapchainKHR m_Swapchain;

    std::vector<VkImage>       m_SwapchainImages;
    std::vector<VkImageView>   m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;

    VkFormat   m_SwapchainImageFormat;
    VkExtent2D m_SwapchainExtent;

    SwapchainSupportDetails m_SwapChainDetails;

    // aka - framebuffer resized
    bool m_SwapchainInvalidated;

    // command pool
    VkCommandPool m_CommandPool;

    // synchronization
    const uint32_t           m_FramesInFlight;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence>     m_Fences;
    std::vector<VkFence>     m_ImagesInFlight;
    size_t                   m_CurrentFrame;

    // Programs
    std::unique_ptr<QuadRendererProgram> m_QuadRendererProgram;

public:
    Renderer(GLFWwindow* windowHandle, uint32_t frames = 2u);
    ~Renderer();

    void BeginFrame();
    void BeginScene();

    // #todo:
    void DrawQuad(const glm::mat4& transform, const glm::vec4& tint);

    void EndScene();
    void EndFrame();

    inline void InvalidateSwapchain() { m_SwapchainInvalidated = true; }

private:
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateWindowSurface(GLFWwindow* windowHandle);

    void CreateSwapchain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateRendererPrograms();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateSynchronizations();

    void RecreateSwapchain();
    void DestroySwapchain();

    uint32_t FetchNextImage();

    void     FilterValidationLayers();
    void     FetchRequiredExtensions();
    void     FetchLogicalDeviceExtensions();
    void     FetchSupportedQueueFamilies();
    void     FetchSwapchainSupportDetails();
    uint32_t FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

    VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessageCallback();
};
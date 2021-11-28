#include "Graphics/Swapchain.h"

#include "Core/Window.h"
#include "Graphics/Device.h"

#include <GLFW/glfw3.h>

Swapchain::Swapchain(Window* window, Device* device, Swapchain* old /* = nullptr */)
    : m_Window(window)
    , m_Device(device)
{
    FetchSwapchainSupportDetails();
    CreateSwapchain(old);
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
}

uint32_t Swapchain::FetchNextImage(VkSemaphore semaphore)
{
    uint32_t index;
    VkResult result = vkAcquireNextImageKHR(m_Device->logical(), m_Swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &index);

    // recreate swap chain if out-of-date/suboptimal
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return UINT32_MAX;
    }
    else if (result != VK_SUCCESS)
    {
        ASSERT(false, "bruh moment")
    }

    return index;
}

Swapchain::~Swapchain()
{
    // destroy framebuffers
    for (auto framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device->logical(), framebuffer, nullptr);

    // destroy render pass
    vkDestroyRenderPass(m_Device->logical(), m_RenderPass, nullptr);

    // destroy image views
    for (auto swapchainImageView : m_ImageViews)
        vkDestroyImageView(m_Device->logical(), swapchainImageView, nullptr);

    // destroy swap chain
    vkDestroySwapchainKHR(m_Device->logical(), m_Swapchain, nullptr);
}

void Swapchain::CreateSwapchain(Swapchain* old)
{
    // pick the desired swap chain surface format
    VkSurfaceFormatKHR swapChainSurfaceFormat {};
    for (const auto& surfaceFormat : m_SwapChainDetails.formats)
    {
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            swapChainSurfaceFormat = surfaceFormat;
    }

    // pick the desired swap chain present mode
    VkPresentModeKHR swapChainPresentMode {};
    for (const auto& presentMode : m_SwapChainDetails.presentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            swapChainPresentMode = presentMode;
    }

    // desired format/mode not found
    if (swapChainSurfaceFormat.format == VK_FORMAT_UNDEFINED)
        swapChainSurfaceFormat = m_SwapChainDetails.formats[0];

    if (swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
        swapChainPresentMode = m_SwapChainDetails.presentModes[0];

    // calculate swapchain extents
    if (m_SwapChainDetails.capabilities.currentExtent.width == UINT32_MAX)
    {
        // fetch window resolution
        int width, height;
        glfwGetFramebufferSize(m_Window->GetHandle(), &width, &height);

        // set extents
        m_Extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        // clamp extents
        m_Extent.width  = std::clamp(m_Extent.width, m_SwapChainDetails.capabilities.minImageExtent.width, m_SwapChainDetails.capabilities.maxImageExtent.width);
        m_Extent.height = std::clamp(m_Extent.height, m_SwapChainDetails.capabilities.minImageExtent.height, m_SwapChainDetails.capabilities.maxImageExtent.height);
    }
    else
        m_Extent = m_SwapChainDetails.capabilities.currentExtent;

    // calculate swapchain image count
    const uint32_t maxImageCount = m_SwapChainDetails.capabilities.maxImageCount;
    const uint32_t minImageCount = m_SwapChainDetails.capabilities.minImageCount;
    uint32_t       imageCount    = maxImageCount > 0 && minImageCount + 1 > maxImageCount ? maxImageCount : minImageCount + 1;

    bool queueFamiliesSameIndex = m_Device->queueIndices().graphics.value() == m_Device->queueIndices().present.value();

    LOG(trace, m_Device->queueIndices().indices.data()[0]);
    LOG(trace, m_Device->queueIndices().indices.data()[1]);
    // std::cout << "Swapchain Indices: \t" << m_Device->queueIndices().indices.data() << std::endl;

    // swapchain create-info
    VkSwapchainCreateInfoKHR swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,

        .surface = m_Device->surface(),

        .minImageCount    = imageCount,
        .imageFormat      = swapChainSurfaceFormat.format,
        .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
        .imageExtent      = m_Extent,
        .imageArrayLayers = 1u,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = !queueFamiliesSameIndex ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,

        .queueFamilyIndexCount = !queueFamiliesSameIndex ? 0u : 2u,
        .pQueueFamilyIndices   = !queueFamiliesSameIndex ? nullptr : m_Device->queueIndices().indices.data(),
        .preTransform          = m_SwapChainDetails.capabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = swapChainPresentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
    };

    // create swap chain
    VKC(vkCreateSwapchainKHR(m_Device->logical(), &swapchainCreateInfo, nullptr, &m_Swapchain));

    // store image format
    m_SwapchainImageFormat = swapChainSurfaceFormat.format;

    // fetch images
    vkGetSwapchainImagesKHR(m_Device->logical(), m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device->logical(), m_Swapchain, &imageCount, m_Images.data());
}

void Swapchain::CreateImageViews()
{
    m_ImageViews.resize(m_Images.size());

    for (int i = 0; i < m_Images.size(); i++)
    {
        // image view create-info
        VkImageViewCreateInfo imageViewCreateInfo {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = m_Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = m_SwapchainImageFormat,
            .components {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0u,
                .levelCount     = 1u,
                .baseArrayLayer = 0u,
                .layerCount     = 1u,
            }
        };

        // create image view
        VKC(vkCreateImageView(m_Device->logical(), &imageViewCreateInfo, nullptr, &m_ImageViews[i]));
    }
}

void Swapchain::CreateRenderPass()
{
    /// attachment description
    VkAttachmentDescription attachmentDescription {
        .format         = m_SwapchainImageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // attachment reference
    VkAttachmentReference attachmentReference {
        .attachment = 0u,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    // subpass description
    VkSubpassDescription subpassDescription {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentReference
    };

    // subpass dependency
    VkSubpassDependency subpassDependency {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0u,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0u,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    // render pass create-info
    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1u,
        .pAttachments    = &attachmentDescription,
        .subpassCount    = 1u,
        .pSubpasses      = &subpassDescription,
        .dependencyCount = 1u,
        .pDependencies   = &subpassDependency,
    };

    // create render pass
    VKC(vkCreateRenderPass(m_Device->logical(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

void Swapchain::CreateFramebuffers()
{
    m_Framebuffers.resize(m_ImageViews.size());

    for (size_t i = 0ull; i < m_ImageViews.size(); i++)
    {
        // framebuffer create-info
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_RenderPass,
            .attachmentCount = 1u,
            .pAttachments    = &m_ImageViews[i],
            .width           = m_Extent.width,
            .height          = m_Extent.height,
            .layers          = 1u
        };

        VKC(vkCreateFramebuffer(m_Device->logical(), &framebufferCreateInfo, nullptr, &m_Framebuffers[i]));
    }
}

void Swapchain::FetchSwapchainSupportDetails()
{
    // fetch device surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->physical(), m_Device->surface(), &m_SwapChainDetails.capabilities);

    // fetch device surface formats
    uint32_t formatCount = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->physical(), m_Device->surface(), &formatCount, nullptr);
    ASSERT(formatCount, "Pipeline::FetchSwapchainSupportDetails: no physical device surface format");

    m_SwapChainDetails.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->physical(), m_Device->surface(), &formatCount, m_SwapChainDetails.formats.data());

    // fetch device surface present modes
    uint32_t presentModeCount = 0u;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->physical(), m_Device->surface(), &presentModeCount, nullptr);
    ASSERT(presentModeCount, "Pipeline::FetchSwapchainSupportDetails: no phyiscal device surface present mode");

    m_SwapChainDetails.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->physical(), m_Device->surface(), &presentModeCount, m_SwapChainDetails.presentModes.data());
}

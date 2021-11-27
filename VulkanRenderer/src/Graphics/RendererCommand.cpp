#include "Graphics/RendererCommand.h"

#include "Graphics/Device.h"

RendererCommand* RendererCommand::s_Instance = nullptr;

void RendererCommand::Init(Device* device)
{
    if (s_Instance)
        delete s_Instance;
    s_Instance = new RendererCommand(device);
}

RendererCommand::RendererCommand(Device* device)
    : m_Device(device)
{
    // command pool create-info
    VkCommandPoolCreateInfo commandpoolCreateInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_Device->graphicsQueueIndex(),
    };

    VKC(vkCreateCommandPool(m_Device->logical(), &commandpoolCreateInfo, nullptr, &m_CommandPool));
}

RendererCommand::~RendererCommand()
{
    vkDestroyCommandPool(m_Device->logical(), m_CommandPool, nullptr);
}

VkCommandBuffer RendererCommand::BeginOneTimeCommandImpl()
{
    VkCommandBufferAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_CommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u,
    };

    VkCommandBuffer commandBuffer;
    VKC(vkAllocateCommandBuffers(m_Device->logical(), &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VKC(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer; // ye wtf visual studio!
}

void RendererCommand::EndOneTimeCommandImpl(VkCommandBuffer* commandBuffer)
{
    VKC(vkEndCommandBuffer(*commandBuffer));

    VkSubmitInfo submitInfo {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1u,
        .pCommandBuffers    = commandBuffer
    };

    VKC(vkQueueSubmit(m_Device->graphicsQueue(), 1u, &submitInfo, VK_NULL_HANDLE));
    VKC(vkQueueWaitIdle(m_Device->graphicsQueue()));

    vkFreeCommandBuffers(m_Device->logical(), m_CommandPool, 1u, commandBuffer);
}

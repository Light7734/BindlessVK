#pragma once

#include "Core/Base.h"

#include <volk.h>

class RendererCommand
{
private:
    static RendererCommand* s_Instance;

    class Device* m_Device;

    VkCommandPool m_CommandPool;

public:
    ~RendererCommand();

    static void Init(class Device* device);

    static VkCommandBuffer BeginOneTimeCommand() { return s_Instance->BeginOneTimeCommandImpl(); }

    static void EndOneTimeCommand(VkCommandBuffer* commandBuffer) { s_Instance->EndOneTimeCommandImpl(commandBuffer); }

private:
    RendererCommand(Device* device);

    VkCommandBuffer BeginOneTimeCommandImpl();
    void            EndOneTimeCommandImpl(VkCommandBuffer* commandBuffer);
};
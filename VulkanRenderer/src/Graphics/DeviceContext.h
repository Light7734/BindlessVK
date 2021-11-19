#pragma once

#include <volk.h>

struct DeviceContext
{
    VkDevice         logical  = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
};
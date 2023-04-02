#pragma once

#define VULKAN_HPP_USE_REFLECT             1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Common/Aliases.hpp"

#include <vk_mem_alloc.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

static_assert(VK_SUCCESS == false, "VK_SUCCESS was supposed to be 0 (false), but it isn't");

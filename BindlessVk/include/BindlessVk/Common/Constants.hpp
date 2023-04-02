#pragma once

#include "BindlessVk/Common/Aliases.hpp"

namespace BINDLESSVK_NAMESPACE {

auto constexpr max_frames_in_flight = usize { 3 };

auto constexpr default_debug_name = str_view { "unnamed" };

auto constexpr desired_swapchain_images = usize { 3 };

} // namespace BINDLESSVK_NAMESPACE

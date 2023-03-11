#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

// @warn do not reorder fields! they have to be contiguous in memory
class Queues
{
public:
	void init_with_gpu(Gpu const &gpu);
	void init_with_device(vk::Device device);

	auto get_create_infos() -> vec<vk::DeviceQueueCreateInfo>;

	auto get_indices() const
	{
		return arr<u32, 2> { graphics_index, present_index };
	}

	auto get_graphics() const
	{
		return graphics;
	}

	auto get_present() const
	{
		return present;
	}

	auto get_graphics_index() const
	{
		return graphics_index;
	}

	auto get_present_index() const
	{
		return present_index;
	}

	auto have_same_index() const
	{
		return graphics_index == present_index;
	}

private:
	vk::Queue graphics;
	vk::Queue present;

	u32 graphics_index;
	u32 present_index;
};

} // namespace BINDLESSVK_NAMESPACE

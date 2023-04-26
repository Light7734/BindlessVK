#pragma once

#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Scene/Scene.hpp"


class UserInterfacePass: public bvk::Renderpass
{
public:
	UserInterfacePass(bvk::VkContext const *vk_context);
	~UserInterfacePass() = default;

	void on_setup() final;
	void on_frame_prepare(u32 frame_index, u32 image_index) final;
	void on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;
	void on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static consteval get_prepare_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_prepare", { 0.5, 1.0, 0.5, 1.0 } };
	}

	auto static consteval get_compute_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_compute", { 0.5, 1.0, 0.5, 1.0 } };
	}

	auto static consteval get_graphics_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_graphics", { 0.5, 1.0, 0.5, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_barrier", { 0.5, 1.0, 0.5, 1.0 } };
	}
};

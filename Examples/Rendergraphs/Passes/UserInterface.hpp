#pragma once

#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Scene/Scene.hpp"


class UserInterfacePass: public bvk::Renderpass
{
public:
	UserInterfacePass(bvk::VkContext *vk_context);
	~UserInterfacePass()
	{
	}

	void on_update(u32 frame_index, u32 image_index) final;
	void on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static consteval get_update_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_update", { 0.5, 1.0, 0.5, 1.0 } };
	}

	auto static consteval get_render_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_render", { 0.5, 1.0, 0.5, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "uipass_barrier", { 0.5, 1.0, 0.5, 1.0 } };
	}
};

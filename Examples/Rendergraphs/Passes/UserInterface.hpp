#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "Framework/Scene/Scene.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>


class UserInterfacePass: public bvk::Renderpass
{
public:
	UserInterfacePass(bvk::VkContext *vk_context);
	~UserInterfacePass()
	{
	}

	virtual void on_update(u32 frame_index, u32 image_index) override;
	virtual void on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) override;
};

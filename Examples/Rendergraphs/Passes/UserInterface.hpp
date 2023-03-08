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

	virtual void on_update(u32 frame_index, u32 image_index) override;
	virtual void on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) override;
};

#pragma once

#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Scene/Scene.hpp"

class Forwardpass: public bvk::Renderpass
{
public:
	Forwardpass(bvk::VkContext *vk_context);
	~Forwardpass()
	{
	}

	virtual void on_update(u32 frame_index, u32 image_index) override;
	virtual void on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) override;

private:
	void draw_model(bvk::Model const *model, vk::CommandBuffer cmd);
};

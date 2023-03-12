#pragma once

#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Scene/Scene.hpp"

class Forwardpass: public bvk::Renderpass
{
public:
	Forwardpass(bvk::VkContext *vk_context);

	~Forwardpass() = default;

	void on_update(u32 frame_index, u32 image_index);

	void on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static consteval get_update_label()
	{
		return vk::DebugUtilsLabelEXT { "fowardpass_update", { 1.0, 0.5, 0.5, 1.0 } };
	}

	auto static consteval get_render_label()
	{
		return vk::DebugUtilsLabelEXT { "forwardpass_render", { 1.0, 0.5, 0.5, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "forwardpass_barrier", { 1.0, 0.5, 0.5, 1.0 } };
	}

private:
	void render_static_meshes();
	void render_skyboxes();

	void render_static_mesh(StaticMeshRendererComponent const &static_mesh, u32 &primitive_index);
	void render_skybox(SkyboxComponent const &skybox);

	void draw_model(bvk::Model const *model, u32 &primitive_index);
	void switch_pipeline(vk::Pipeline pipeline);

private:
	Scene *scene = {};
	vk::Pipeline current_pipeline = {};
	vk::CommandBuffer cmd;
};

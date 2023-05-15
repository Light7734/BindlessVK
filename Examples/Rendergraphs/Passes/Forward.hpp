#pragma once

#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Scene/Scene.hpp"

class Forwardpass: public bvk::Renderpass
{
public:
	struct UserData
	{
		Scene *scene;
		bvk::MemoryAllocator *memory_allocator;

		bvk::ShaderPipeline *model_pipeline;
		bvk::ShaderPipeline *skybox_pipeline;
		bvk::ShaderPipeline *cull_pipeline;
	};

	struct ModelInformation
	{
		glm::mat4 model;

		glm::vec3 position;
		f32 rotation;

		i32 albedo_texture_index;
		i32 normal_texture_index;
		i32 metallic_roughness_texture_index;

		u32 first_index;
		u32 index_count;

		u32 _pad0;
		u32 _pad1;
		u32 _pad2;
	};

public:
	Forwardpass(bvk::VkContext const *vk_context);

	~Forwardpass() = default;

	void on_setup(bvk::Rendergraph *graph) final;

	void on_frame_prepare(u32 frame_index, u32 image_index) final;
	void on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;
	void on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static consteval get_prepare_label()
	{
		return vk::DebugUtilsLabelEXT { "fowardpass_prepare", { 1.0, 0.5, 0.5, 1.0 } };
	}

	auto static consteval get_compute_label()
	{
		return vk::DebugUtilsLabelEXT { "forwardpass_compute", { 1.0, 0.5, 0.5, 1.0 } };
	}

	auto static consteval get_graphics_label()
	{
		return vk::DebugUtilsLabelEXT { "forwardpass_graphics", { 1.0, 0.5, 0.5, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "forwardpass_barrier", { 1.0, 0.5, 0.5, 1.0 } };
	}

private:
	void render_static_meshes(u32 frame_index);
	void render_skyboxes();

	void render_static_mesh(StaticMeshRendererComponent const &static_mesh, u32 &primitive_index);
	void render_skybox(SkyboxComponent const &skybox);

	void draw_model(bvk::Model const *model, u32 &primitive_index);
	void switch_pipeline(vk::Pipeline pipeline);

private:
	bvk::MemoryAllocator *memory_allocator = {};
	Scene *scene = {};
	bvk::Device *device = {};

	bvk::Buffer const *draw_indirect_buffer = {};

	vk::Pipeline current_pipeline = {};

	vk::CommandBuffer cmd = {};

	usize static_mesh_count = {};

	bvk::ShaderPipeline *cull_pipeline = {};
	bvk::ShaderPipeline *model_pipeline = {};
	bvk::ShaderPipeline *skybox_pipeline = {};

	u32 primitive_count = {};

	bool freeze_cull = {};
};

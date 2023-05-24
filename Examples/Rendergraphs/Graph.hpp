#pragma once

#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

class InputBuffer
{
public:
	InputBuffer() {};

	virtual u32 get_compute_binding()
	{
		return std::numeric_limits<u32>::max();
	}

	virtual u32 get_graphics_binding()
	{
		return std::numeric_limits<u32>::max();
	}

	/** Returns null-terminated str view to debug_name */
	str_view get_name()
	{
		return str_view(debug_name);
	}

private:
	str debug_name;
};

class BasicRendergraph: public bvk::RenderNode
{
public:
	struct UserData
	{
		Scene *scene;
		bvk::FragmentedBuffer *vertex_buffer;
		bvk::FragmentedBuffer *index_buffer;
		bvk::MemoryAllocator const *memory_allocator;
		bvk::DebugUtils *debug_utils;
	};

	struct U_FrameData
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec4 view_position;

		auto static constexpr name = str { "frame_data" };
		auto static constexpr key = hash_str("frame_data");

		auto static constexpr binding = usize { 0 };
	};

	struct U_SceneData
	{
		glm::vec4 light_position;
		u32 primitive_count;

		auto static constexpr name = str { "scene_data" };
		auto static constexpr key = hash_str("scene_data");

		auto static constexpr binding = usize { 1 };
	};

	struct U_ModelData
	{
		f32 x;
		f32 y;
		f32 z;
		f32 r;

		i32 albedo_texture_index;
		i32 normal_texture_index;
		i32 metallic_roughness_texture_index;
		i32 _0;

		glm::mat4 model;

		auto static constexpr name = str { "model_data" };
		auto static constexpr key = hash_str("model_data");

		auto static constexpr binding = usize { 2 };
	};

	struct U_DrawIndirect
	{
		vk::DrawIndexedIndirectCommand cmd;

		auto static constexpr name = str { "draw_indirect" };
		auto static constexpr key = hash_str("draw_indirect");

		auto static constexpr binding = usize { 3 };
	};

	struct U_Textures
	{
		auto static constexpr name = str { "textures" };
		auto static constexpr binding = usize { 4 };
	};

	struct U_TextureCubes
	{
		auto static constexpr name = str { "texture_cubes" };
		auto static constexpr binding = usize { 5 };
	};

	auto static constexpr graphics_descriptor_set_bindings_count = usize { 5 };
	auto static constexpr compute_descriptor_set_bindings_count = usize { 4 };

public:
	BasicRendergraph() = default;

	BasicRendergraph(bvk::VkContext const *const vk_context);

	BasicRendergraph(BasicRendergraph &&other) = default;
	BasicRendergraph &operator=(BasicRendergraph &&other) = default;

	~BasicRendergraph() = default;

	void on_setup(RenderNode *parent) final;

	void on_frame_prepare(u32 frame_index, u32 image_index) final;

	void on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final
	{
	}

	void on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static get_graphics_descriptor_set_bindings() -> pair<
	    arr<vk::DescriptorSetLayoutBinding, graphics_descriptor_set_bindings_count>,
	    arr<vk::DescriptorBindingFlags, graphics_descriptor_set_bindings_count>>;

	auto static get_compute_descriptor_set_bindings() -> pair<
	    arr<vk::DescriptorSetLayoutBinding, compute_descriptor_set_bindings_count>,
	    arr<vk::DescriptorBindingFlags, compute_descriptor_set_bindings_count>>;

	auto static consteval get_prepare_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_prepare", { 1.0, 1.0, 1.0, 1.0 } };
	}

	auto static consteval get_compute_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_compute", { 1.0, 1.0, 1.0, 1.0 } };
	}

	auto static consteval get_graphics_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_graphics", { 1.0, 1.0, 1.0, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_barrier", { 1.0, 1.0, 1.0, 1.0 } };
	}

private:
	void upload_model_data_to_gpu();
	void upload_indirect_data_to_gpu();

	void bind_graphic_buffers(vk::CommandBuffer cmd);

	void stage_indirect(U_DrawIndirect *buffer_map);

	void update_descriptor_sets();

	void update_for_cameras();
	void update_for_lights();
	void stage_model(U_ModelData *buffer_map, u32 buffer_index);
	void update_for_skyboxes();

	void update_for_camera(TransformComponent const &transform, CameraComponent const &camera);
	void update_for_light(TransformComponent const &transform, LightComponent const &camera);
	void update_for_model(
	    U_ModelData *buffer_map,
	    u32 buffer_index,
	    TransformComponent const &transform,
	    StaticMeshRendererComponent const &static_mesh,
	    u32 &primitive_index
	);
	void update_for_skybox(SkyboxComponent const &skybox);

private:
	bvk::Device *device = {};
	bvk::FragmentedBuffer *vertex_buffer = {};
	bvk::FragmentedBuffer *index_buffer = {};
	bvk::DebugUtils *debug_util = {};

	bvk::Buffer staging_buffer = {};

	usize primitive_count = {};

	u32 frame_index = {};
	Scene *scene = {};
	vec<vk::WriteDescriptorSet> descriptor_writes = {};
};

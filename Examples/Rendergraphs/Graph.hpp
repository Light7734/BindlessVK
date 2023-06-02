#pragma once

#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/Timer.hpp"

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

	struct DirectionalLight
	{
		glm::vec4 direction;

		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
	};

	struct PointLight
	{
		glm::vec4 position;

		f32 constant;
		f32 linear;
		f32 quadratic;
		f32 _0;

		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;

		auto static constexpr max_count = 32;
	};

	struct RenderScene
	{
		DirectionalLight directional_light;

		arr<PointLight, PointLight::max_count> point_lights;
		u32 point_light_count;

		u32 primitive_count;
	};

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 view_projection;
		glm::vec4 view_position;
	};

	// Descriptors

	struct FrameDescriptor
	{
		Camera camera;
		RenderScene render_scene;

		f32 delta_time;

		auto static constexpr name = str { "frame_data" };
		auto static constexpr key = hash_str("frame_data");

		auto static constexpr binding = usize { 0 };
	};

	struct PrimitivesDescriptor
	{
		f32 x;
		f32 y;
		f32 z;
		f32 r;

		i32 albedo_index;
		i32 normal_index;
		i32 mr_index;
		i32 _0;

		glm::mat4 transform;

		auto static constexpr name = str { "model_data" };
		auto static constexpr key = hash_str(name);

		auto static constexpr binding = usize { 1 };
	};

	struct DrawIndirectDescriptor
	{
		vk::DrawIndexedIndirectCommand cmd;
		u32 _0;
		u32 _1;
		u32 _2;

		auto static constexpr name = str { "draw_indirect" };
		auto static constexpr key = hash_str(name);

		auto static constexpr binding = usize { 2 };
	};

	struct TexturesDescriptor
	{
		auto static constexpr name = str { "textures" };
		auto static constexpr binding = usize { 3 };
	};

	struct TextureCubesDescriptor
	{
		auto static constexpr name = str { "texture_cubes" };
		auto static constexpr binding = usize { 4 };
	};

	auto static constexpr graphics_descriptor_set_bindings_count = usize { 4 };
	auto static constexpr compute_descriptor_set_bindings_count = usize { 3 };

public:
	BasicRendergraph() = default;

	BasicRendergraph(bvk::VkContext const *const vk_context);

	BasicRendergraph(BasicRendergraph &&other) = default;

	BasicRendergraph &operator=(BasicRendergraph &&other) = default;

	~BasicRendergraph() = default;

	void on_setup(RenderNode *parent) final;

	void on_frame_prepare(u32 frame_index, u32 image_index) final;

	void on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

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
	void setup_descriptors();

	void setup_descriptor_maps();

	void setup_primitives_descriptor();
	void setup_draw_indirects_descriptor();

	void setup_frame_descriptor_maps();

	void update_frame();

	void update_delta_time();

	void bind_graphics_buffers(vk::CommandBuffer cmd);

	void stage_indirect();

	void update_descriptors();

	void update_cameras();
	void update_skyboxes();
	void update_directional_lights();
	void update_point_lights();

	void stage_static_meshes(u32 buffer_index);

	void update_camera(TransformComponent const &transform, CameraComponent const &camera);

	void update_directional_light(DirectionalLightComponent const &directional_light);

	void update_point_light(
	    TransformComponent const &transform,
	    PointLightComponent const &point_light,
	    u32 index
	);

	void stage_static_mesh(
	    u32 buffer_index,
	    TransformComponent const &transform,
	    StaticMeshComponent const &static_mesh,
	    u32 &primitive_index
	);
	void update_skybox(SkyboxComponent const &skybox);

	void update_primitive_buffer(
	    PrimitivesDescriptor &primitive,

	    TransformComponent const &transform,
	    bvk::Model::MaterialParameters const &material
	);

	void update_primitive_textures(
	    bvk::DescriptorSet const &descriptor_set,
	    vec<bvk::Texture> const &textures,
	    bvk::Model::MaterialParameters material
	);

private:
	bvk::Device *device = {};
	bvk::FragmentedBuffer *vertex_buffer = {};
	bvk::FragmentedBuffer *index_buffer = {};
	bvk::DebugUtils *debug_util = {};

	bvk::Buffer staging_buffer = {};

	f32 linear = 0.09f;
	f32 quadratic = 0.032f;

	arr<FrameDescriptor *, bvk::max_frames_in_flight> frame_descriptor_maps;
	arr<DrawIndirectDescriptor *, bvk::max_frames_in_flight> draw_indirect_descriptor_maps;
	PrimitivesDescriptor *primitive_descriptor_map;
	void *staging_buffer_map;

	usize primitive_count = {};

	u32 frame_index = {};
	Scene *scene = {};
	vec<vk::WriteDescriptorSet> descriptor_writes = {};

	Timer timer = {};
};

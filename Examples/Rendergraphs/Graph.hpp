#pragma once

#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

class BasicRendergraph: public bvk::Rendergraph
{
public:
	struct UserData
	{
		Scene *scene;
		bvk::FragmentedBuffer *vertex_buffer;
	};

	struct FrameData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 view_position;

		auto static constexpr binding = usize { 0 };
	};

	struct SceneData
	{
		glm::vec4 light_position;

		auto static constexpr binding = usize { 1 };
	};

	struct ObjectData
	{
		int albedo_texture_index;
		int normal_texture_index;
		int metallic_roughness_texture_index;
		int _;
		glm::mat4 model;

		auto static constexpr binding = usize { 2 };
	};

	struct U_Textures
	{
		auto static constexpr binding = usize { 3 };
	};

	struct U_TextureCubes
	{
		auto static constexpr binding = usize { 4 };
	};

public:
	BasicRendergraph(bvk::VkContext const *const vk_context);

	~BasicRendergraph() = default;

	void on_update(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) final;

	auto static get_descriptor_set_bindings()
	    -> pair<arr<vk::DescriptorSetLayoutBinding, 5>, arr<vk::DescriptorBindingFlags, 5>>;

	auto static consteval get_update_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_update", { 1.0, 1.0, 1.0, 1.0 } };
	}

	auto static consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_barrier", { 1.0, 1.0, 1.0, 1.0 } };
	}

private:
	void bind_buffers(vk::CommandBuffer cmd);

	void update_descriptor_sets();

	void update_for_cameras();
	void update_for_lights();
	void update_for_meshes();
	void update_for_skyboxes();

	void update_for_camera(TransformComponent const &transform, CameraComponent const &camera);
	void update_for_light(TransformComponent const &transform, LightComponent const &camera);
	void update_for_mesh(
	    TransformComponent const &transform,
	    StaticMeshRendererComponent const &static_mesh,
	    u32 &primitive_index
	);
	void update_for_skybox(SkyboxComponent const &skybox);

private:
	bvk::FragmentedBuffer *vertex_buffer = {};

	u32 frame_index = {};
	Scene *scene = {};
	vec<vk::WriteDescriptorSet> descriptor_writes = {};
};

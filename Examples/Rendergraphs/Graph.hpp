#pragma once

#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

class BasicRendergraph: public bvk::Rendergraph
{
public:
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

public:
	BasicRendergraph(ref<bvk::VkContext> vk_context);

	static auto get_descriptor_set_bindings()
	    -> pair<arr<vk::DescriptorSetLayoutBinding, 4>, arr<vk::DescriptorBindingFlags, 4>>;

	auto static inline consteval get_update_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_update", { 1.0, 1.0, 1.0, 1.0 } };
	}

	auto static inline consteval get_barrier_label()
	{
		return vk::DebugUtilsLabelEXT { "graph_barrier", { 1.0, 1.0, 1.0, 1.0 } };
	}

	void virtual on_update(u32 frame_index, u32 image_index) override;

private:
	void update_descriptor_sets();

	void update_for_cameras();
	void update_for_lights();
	void update_for_meshes();
	void update_for_skyboxes();

	void update_for_camera(TransformComponent const &transform, CameraComponent const &camera);
	void update_for_light(TransformComponent const &transform, LightComponent const &camera);
	void update_for_mesh(StaticMeshRendererComponent const &static_mesh);
	void update_for_skybox(SkyboxComponent const &skybox);

private:
	u32 frame_index = {};
	Scene *scene = {};
	vec<vk::WriteDescriptorSet> descriptor_writes = {};
};

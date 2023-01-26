#pragma once

#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec4 viewPos;
};

// @todo:
struct SceneData
{
	glm::vec4 light_pos;
};

inline void render_graph_update(
  bvk::VkContext* vk_context,
  bvk::RenderGraph* render_graph,
  uint32_t frame_index,
  void* user_pointer
)
{
	auto* scene = reinterpret_cast<Scene*>(user_pointer);

	scene->group(entt::get<TransformComponent, CameraComponent>)
	  .each([&](TransformComponent& transformComp, CameraComponent& cameraComp) {
		  *(CameraData*)render_graph->map_descriptor_buffer("frame_data", frame_index) = {
			  .projection = cameraComp.GetProjection(),
			  .view = cameraComp.GetView(transformComp.translation),
			  .viewPos = glm::vec4(transformComp.translation, 1.0f),
		  };
		  render_graph->unmap_descriptor_buffer("frame_data");
	  });

	scene->group(entt::get<TransformComponent, LightComponent>)
	  .each([&](TransformComponent& trasnformComp, LightComponent& lightComp) {
		  SceneData* sceneData = (SceneData*)
		                           render_graph->map_descriptor_buffer("scene_data", frame_index);

		  sceneData[0] = {
			  .light_pos = glm::vec4(trasnformComp.translation, 1.0f),
		  };
		  render_graph->unmap_descriptor_buffer("scene_data");
	  });
}

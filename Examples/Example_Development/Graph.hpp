#pragma once

#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

u32 constexpr const graph_frame_data_set_index = 0;
u32 constexpr const graph_scene_data_set_index = 1;

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
    bvk::VkContext *vk_context,
    bvk::RenderGraph *render_graph,
    uint32_t frame_index,
    void *user_pointer
)
{
	auto const device = vk_context->get_device();

	auto *scene = reinterpret_cast<Scene *>(user_pointer);

	auto &frame_data_buffer = render_graph->buffer_inputs[graph_frame_data_set_index];
	auto &scene_data_buffer = render_graph->buffer_inputs[graph_scene_data_set_index];

	auto *const frame_data = reinterpret_cast<CameraData *>(frame_data_buffer.map_block(frame_index)
	);
	auto *const scene_data = reinterpret_cast<SceneData *>(scene_data_buffer.map_block(frame_index)
	);

	scene->group(entt::get<TransformComponent, CameraComponent>)
	    .each([&](TransformComponent &transformComp, CameraComponent &cameraComp) {
		    *frame_data = {
			    .projection = cameraComp.GetProjection(),
			    .view = cameraComp.GetView(transformComp.translation),
			    .viewPos = glm::vec4(transformComp.translation, 1.0f),
		    };
	    });

	scene->group(entt::get<TransformComponent, LightComponent>)
	    .each([&](TransformComponent &trasnformComp, LightComponent &lightComp) {
		    *scene_data = {
			    .light_pos = glm::vec4(trasnformComp.translation, 1.0f),
		    };
	    });

	scene_data_buffer.unmap();
	frame_data_buffer.unmap();
}

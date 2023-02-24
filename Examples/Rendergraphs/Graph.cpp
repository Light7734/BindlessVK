#include "Rendergraphs/Graph.hpp"

#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

BasicRendergraph::BasicRendergraph(ref<bvk::VkContext> const vk_context)
    : bvk::Rendergraph(vk_context)
{
}

void BasicRendergraph::on_update(u32 const frame_index, u32 const image_index)
{
	auto const device = vk_context->get_device();

	auto *scene = any_cast<Scene *>(user_data);

	auto &frame_data_buffer = buffer_inputs[FrameData::binding];
	auto &scene_data_buffer = buffer_inputs[SceneData::binding];

	auto *const frame_data = reinterpret_cast<FrameData *>(frame_data_buffer.map_block(frame_index)
	);

	auto *const scene_data = reinterpret_cast<SceneData *>(scene_data_buffer.map_block(frame_index)
	);

	scene->group(entt::get<TransformComponent, CameraComponent>)
	    .each([&](TransformComponent &transform_comp, CameraComponent &camera_comp) {
		    *frame_data = {
			    .projection = camera_comp.get_projection(),
			    .view = camera_comp.get_view(transform_comp.translation),
			    .viewPos = glm::vec4(transform_comp.translation, 1.0f),
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

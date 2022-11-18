#pragma once

#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Core/Common.hpp"
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
	glm::vec4 lightPos;
};

inline void RenderGraphUpdate(const bvk::RenderContext& context)
{
	Scene* scene = (Scene*)context.userPointer;

	scene->group(entt::get<TransformComponent, CameraComponent>).each([&](TransformComponent& transformComp, CameraComponent& cameraComp) {
		*(CameraData*)context.graph->MapDescriptorBuffer("frame_data", context.frameIndex) = {
			.projection = cameraComp.GetProjection(),
			.view       = cameraComp.GetView(transformComp.translation),
			.viewPos    = glm::vec4(transformComp.translation, 1.0f),
		};
		context.graph->UnMapDescriptorBuffer("frame_data");
	});

	scene->group(entt::get<TransformComponent, LightComponent>).each([&](TransformComponent& trasnformComp, LightComponent& lightComp) {
		SceneData* sceneData = (SceneData*)context.graph->MapDescriptorBuffer("scene_data", context.frameIndex);

		sceneData[0] = {
			.lightPos = glm::vec4(trasnformComp.translation, 1.0f),
		};
		context.graph->UnMapDescriptorBuffer("scene_data");
	});
}

#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Scene/Scene.hpp"

inline void ForwardPassUpdate(const bvk::RenderContext& context)
{
	Scene* scene = (Scene*)context.userPointer;

	vk::DescriptorSet descriptorSet = context.pass->descriptorSets[context.frameIndex];

	// @todo: don't update every frame
	scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>).each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
		for (const auto* node : renderComp.model->nodes)
		{
			for (const auto& primitive : node->mesh)
			{
				const auto& model    = renderComp.model;
				const auto& material = model->materialParameters[primitive.materialIndex];

				uint32_t albedoTextureIndex            = material.albedoTextureIndex;
				uint32_t normalTextureIndex            = material.normalTextureIndex;
				uint32_t metallicRoughnessTextureIndex = material.metallicRoughnessTextureIndex;

				bvk::Texture* albedoTexture            = model->textures[albedoTextureIndex];
				bvk::Texture* normalTexture            = model->textures[normalTextureIndex];
				bvk::Texture* metallicRoughnessTexture = model->textures[metallicRoughnessTextureIndex];

				std::vector<vk::WriteDescriptorSet> descriptorWrites = {

					vk::WriteDescriptorSet {
					    descriptorSet,
					    0ul,
					    albedoTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &albedoTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					},

					vk::WriteDescriptorSet {
					    descriptorSet,
					    0ul,
					    metallicRoughnessTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &metallicRoughnessTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					},

					vk::WriteDescriptorSet {
					    descriptorSet,
					    0ul,
					    normalTextureIndex,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &normalTexture->descriptorInfo,
					    nullptr,
					    nullptr,
					}
				};

				context.logicalDevice.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0ull, nullptr);
			}
		}
	});
}

inline void ForwardPassRender(const bvk::RenderContext& context)
{
	VkDeviceSize offset { 0 };
	vk::Pipeline currentPipeline = VK_NULL_HANDLE;

	Scene* scene = (Scene*)context.userPointer;

	scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>).each([&](TransformComponent& transformComp, StaticMeshRendererComponent& renderComp) {
		const auto cmd = context.cmd;

		// bind pipeline
		vk::Pipeline newPipeline = renderComp.material->base->shader->pipeline;
		if (currentPipeline != newPipeline)
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, newPipeline);
			currentPipeline = newPipeline;
		}
		// bind buffers
		cmd.bindVertexBuffers(0, 1, renderComp.model->vertexBuffer->GetBuffer(), &offset);
		cmd.bindIndexBuffer(*(renderComp.model->indexBuffer->GetBuffer()), 0u, vk::IndexType::eUint32);

		// draw primitves
		for (const auto* node : renderComp.model->nodes)
		{
			for (const auto& primitive : node->mesh)
			{
				uint32_t textureIndex = renderComp.model->materialParameters[primitive.materialIndex].albedoTextureIndex;
				cmd.drawIndexed(primitive.indexCount, 1ull, primitive.firstIndex, 0ull, textureIndex); // @todo: lol fix this garbage
			}
		}
	});
}

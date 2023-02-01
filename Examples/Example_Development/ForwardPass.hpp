#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "Framework/Scene/Scene.hpp"

using u32 = u32;

inline void forward_pass_update(
  bvk::VkContext *vk_context,
  bvk::RenderGraph *render_graph,
  bvk::Renderpass *render_pass,
  u32 frame_index,
  void *user_pointer
)
{
	auto const device = vk_context->get_device();
	auto const descriptor_set = render_pass->descriptor_sets[frame_index];

	auto *scene = reinterpret_cast<Scene *>(user_pointer);

	// @todo: don't update every frame
	auto const renderables = scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>);

	auto descriptor_writes = vec<vk::WriteDescriptorSet> {};

	renderables.each([&](auto const &transform_component, auto const &render_component) {
		for (auto const *const node : render_component.model->nodes) {
			for (const auto &primitive : node->mesh) {
				auto const &model = render_component.model;
				auto const &material = model->material_parameters[primitive.material_index];

				u32 albedo_texture_index = material.albedo_texture_index;
				u32 normal_texture_index = material.normal_texture_index;
				u32 metallic_roughness_texture_index = material.metallic_roughness_texture_index;

				auto const *const albedo_texture = &model->textures[albedo_texture_index];
				auto const *const normal_texture = &model->textures[normal_texture_index];
				auto const *const metallic_roughness_texture = &model->textures
				                                                  [metallic_roughness_texture_index];

				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				  descriptor_set,
				  0ul,
				  albedo_texture_index,
				  1ul,
				  vk::DescriptorType::eCombinedImageSampler,
				  &albedo_texture->descriptor_info,
				  nullptr,
				  nullptr,
				});
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				  descriptor_set,
				  0ul,
				  metallic_roughness_texture_index,
				  1ul,
				  vk::DescriptorType::eCombinedImageSampler,
				  &metallic_roughness_texture->descriptor_info,
				  nullptr,
				  nullptr,
				});
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				  descriptor_set,
				  0ul,
				  normal_texture_index,
				  1ul,
				  vk::DescriptorType::eCombinedImageSampler,
				  &normal_texture->descriptor_info,
				  nullptr,
				  nullptr,
				});
			};
		}
	});

	device.updateDescriptorSets(descriptor_writes, 0);
}


inline void draw_model(bvk::Model *model, vk::CommandBuffer cmd)
{
	auto const offset = VkDeviceSize { 0 };

	// bind buffers
	cmd.bindVertexBuffers(0, 1, model->vertex_buffer->get_buffer(), &offset);
	cmd.bindIndexBuffer(*(model->index_buffer->get_buffer()), 0u, vk::IndexType::eUint32);

	for (auto const *node : model->nodes) {
		for (auto const &primitive : node->mesh) {
			u32 texture_index = model->material_parameters[primitive.material_index].albedo_texture_index;

			// @todo: fix this hack
			cmd.drawIndexed(primitive.index_count, 1ull, primitive.first_index, 0ull, texture_index);
		}
	}
}

inline void forward_pass_render(
  bvk::VkContext *vk_context,
  bvk::RenderGraph *render_graph,
  bvk::Renderpass *render_pass,
  vk::CommandBuffer cmd,
  u32 frame_index,
  u32 image_index,
  void *user_pointer
)
{
	const auto device = vk_context->get_device();
	const auto &surface = vk_context->get_surface();

	auto pipeline = vk::Pipeline {};

	auto *const scene = reinterpret_cast<Scene *>(user_pointer);
	auto const renderables = scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>);

	renderables.each([&](auto const &transform_component, auto const &render_component) {
		auto const material = render_component.material;
		auto const effect = material->get_effect();
		auto const new_pipeline = effect->get_pipeline();

		if (pipeline != new_pipeline) {
			// bind new pipeline
			pipeline = new_pipeline;
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

			// set dynamic states
			cmd.setScissor(0, { vk::Rect2D { { 0, 0 }, surface.framebuffer_extent } });
			cmd.setViewport(
			  0,
			  {
			    vk::Viewport {
			      0.0f,
			      0.0f,
			      static_cast<f32>(surface.framebuffer_extent.width),
			      static_cast<f32>(surface.framebuffer_extent.height),
			      0.0f,
			      1.0f,
			    },
			  }
			);
		}

		draw_model(render_component.model, cmd);
	});
}

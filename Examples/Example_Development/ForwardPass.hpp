#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "Framework/Scene/Scene.hpp"

using u32 = uint32_t;

inline void forward_pass_update(
  bvk::Device* device,
  bvk::RenderGraph* render_graph,
  bvk::Renderpass* render_pass,
  uint32_t frame_index,
  void* user_pointer
)
{
	Scene* scene = (Scene*)user_pointer;

	vk::DescriptorSet descriptor_set = render_pass->descriptor_sets[frame_index];

	// @todo: don't update every frame
	scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>)
	  .each([&](TransformComponent& transform_comp, StaticMeshRendererComponent& render_comp) {
		  for (const auto* node : render_comp.model->nodes) {
			  for (const auto& primitive : node->mesh) {
				  const auto& model    = render_comp.model;
				  const auto& material = model->material_parameters[primitive.material_index];

				  uint32_t albedo_texture_index             = material.albedo_texture_index;
				  uint32_t normal_texture_index             = material.normal_texture_index;
				  uint32_t metallic_roughness_texture_index = material.metallic_roughness_texture_index;

				  bvk::Texture* albedo_texture             = model->textures[albedo_texture_index];
				  bvk::Texture* normal_texture             = model->textures[normal_texture_index];
				  bvk::Texture* metallic_roughness_texture = model
				                                               ->textures[metallic_roughness_texture_index];

				  std::vector<vk::WriteDescriptorSet> descriptor_writes = {
					  vk::WriteDescriptorSet {
					    descriptor_set,
					    0ul,
					    albedo_texture_index,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &albedo_texture->descriptor_info,
					    nullptr,
					    nullptr,
					  },
					  vk::WriteDescriptorSet {
					    descriptor_set,
					    0ul,
					    metallic_roughness_texture_index,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &metallic_roughness_texture->descriptor_info,
					    nullptr,
					    nullptr,
					  },
					  vk::WriteDescriptorSet {
					    descriptor_set,
					    0ul,
					    normal_texture_index,
					    1ul,
					    vk::DescriptorType::eCombinedImageSampler,
					    &normal_texture->descriptor_info,
					    nullptr,
					    nullptr,
					  }
				  };

				  device->logical.updateDescriptorSets(
				    static_cast<uint32_t>(descriptor_writes.size()),
				    descriptor_writes.data(),
				    0ull,
				    nullptr
				  );
			  }
		  }
	  });
}


inline void draw_model(bvk::Model* model, vk::CommandBuffer cmd)
{
	static const VkDeviceSize offset { 0 };

	// bind buffers
	cmd.bindVertexBuffers(0, 1, model->vertex_buffer->get_buffer(), &offset);
	cmd.bindIndexBuffer(*(model->index_buffer->get_buffer()), 0u, vk::IndexType::eUint32);

	for (const auto* node : model->nodes) {
		for (const auto& primitive : node->mesh) {
			u32 texture_index = model->material_parameters[primitive.material_index].albedo_texture_index;

			// @todo: fix this hack
			cmd.drawIndexed(primitive.index_count, 1ull, primitive.first_index, 0ull, texture_index);
		}
	}
}

inline void forward_pass_render(
  bvk::Device* device,
  bvk::RenderGraph* render_graph,
  bvk::Renderpass* render_pass,
  vk::CommandBuffer cmd,
  uint32_t frame_index,
  uint32_t image_index,
  void* user_pointer
)
{
	vk::Pipeline pipeline = VK_NULL_HANDLE;

	Scene* scene = (Scene*)user_pointer;
	scene->group(entt::get<TransformComponent, StaticMeshRendererComponent>)
	  .each([&](TransformComponent& transform_comp, StaticMeshRendererComponent& render_comp) {
		  vk::Pipeline new_pipeline = render_comp.material->shader_pass->pipeline;
		  if (pipeline != new_pipeline) {
			  // bind new pipeline
			  pipeline = new_pipeline;
			  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

			  // set dynamic states
			  cmd.setScissor(0, { vk::Rect2D { { 0, 0 }, device->framebuffer_extent } });
			  cmd.setViewport(
			    0,
			    {
			      vk::Viewport {
			        0.0f,
			        0.0f,
			        (float)device->framebuffer_extent.width,
			        (float)device->framebuffer_extent.height,
			        0.0f,
			        1.0f,
			      },
			    }
			  );
		  }
		  draw_model(render_comp.model, cmd);
	  });
}

#include "Rendergraphs/Passes/Forward.hpp"

Forwardpass::Forwardpass(bvk::VkContext *const vk_context): bvk::Renderpass(vk_context)
{
}

void Forwardpass::on_update(u32 frame_index, u32 image_index)
{
	auto const device = vk_context->get_device();
	auto const descriptor_set = descriptor_sets[frame_index];

	auto const *const scene = any_cast<Scene *>(user_data);

	// @todo: don't update every frame
	auto const renderables = scene->view<const StaticMeshRendererComponent>();

	auto descriptor_writes = vec<vk::WriteDescriptorSet> {};

	renderables.each([&](auto const &render_component) {
		for (auto const *const node : render_component.model->get_nodes()) {
			for (auto const &primitive : node->mesh) {
				auto const &model = render_component.model;
				auto const &material = model->get_material_parameters()[primitive.material_index];
				auto const &textures = model->get_textures();

				auto const albedo_texture_index = material.albedo_texture_index;
				auto const normal_texture_index = material.normal_texture_index;
				auto const metallic_roughness_texture_index = material
				                                                  .metallic_roughness_texture_index;


				if (albedo_texture_index != -1) {
					auto const *const albedo_texture = &textures[albedo_texture_index];
					descriptor_writes.emplace_back(
					    descriptor_set,
					    0,
					    albedo_texture_index,
					    1,
					    vk::DescriptorType::eCombinedImageSampler,
					    albedo_texture->get_descriptor_info()
					);
				}

				if (metallic_roughness_texture_index != -1) {
					auto const *const metallic_roughness_texture
					    = &textures[metallic_roughness_texture_index];
					descriptor_writes.emplace_back(vk::WriteDescriptorSet {
					    descriptor_set,
					    0,
					    metallic_roughness_texture_index,
					    1,
					    vk::DescriptorType::eCombinedImageSampler,
					    metallic_roughness_texture->get_descriptor_info(),
					});
				}

				if (normal_texture_index != -1) {
					auto const *const normal_texture = &textures[normal_texture_index];
					descriptor_writes.emplace_back(vk::WriteDescriptorSet {
					    descriptor_set,
					    0,
					    normal_texture_index,
					    1,
					    vk::DescriptorType::eCombinedImageSampler,
					    normal_texture->get_descriptor_info(),
					});
				}
			};
		}
	});

	device.updateDescriptorSets(descriptor_writes, 0);
}

void Forwardpass::on_render(
    vk::CommandBuffer const cmd,
    u32 const frame_index,
    u32 const image_index
)
{
	auto const device = vk_context->get_device();
	auto const &surface = vk_context->get_surface();

	auto pipeline = vk::Pipeline {};

	auto const *const scene = any_cast<Scene *>(user_data);
	auto const renderables = scene->view<const StaticMeshRendererComponent>();

	renderables.each([&](auto const &render_component) {
		auto const material = render_component.material;
		auto const effect = material->get_effect();
		auto const new_pipeline = effect->get_pipeline();

		if (pipeline != new_pipeline) {
			// bind new pipeline
			pipeline = new_pipeline;
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

			// set dynamic states
			cmd.setScissor(0, { vk::Rect2D { { 0, 0 }, surface.get_framebuffer_extent() } });
			cmd.setViewport(
			    0,
			    {
			        vk::Viewport {
			            0.0f,
			            0.0f,
			            static_cast<f32>(surface.get_framebuffer_extent().width),
			            static_cast<f32>(surface.get_framebuffer_extent().height),
			            0.0f,
			            1.0f,
			        },
			    }
			);
		}

		draw_model(render_component.model, cmd);
	});
}

void Forwardpass::draw_model(bvk::Model const *const model, vk::CommandBuffer const cmd)
{
	auto const offset = VkDeviceSize { 0 };

	// bind buffers
	cmd.bindVertexBuffers(0, 1, model->get_vertex_buffer()->get_buffer(), &offset);
	cmd.bindIndexBuffer(*(model->get_index_buffer()->get_buffer()), 0u, vk::IndexType::eUint32);

	for (auto const *node : model->get_nodes()) {
		for (auto const &primitive : node->mesh) {
			auto const &material_parameters = model->get_material_parameters();
			auto const texture_index = material_parameters[primitive.material_index]
			                               .albedo_texture_index;

			// @todo: fix this hack
			cmd.drawIndexed(
			    primitive.index_count,
			    1ull,
			    primitive.first_index,
			    0ull,
			    texture_index
			);
		}
	}
}

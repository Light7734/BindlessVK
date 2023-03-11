#include "Rendergraphs/Passes/Forward.hpp"

Forwardpass::Forwardpass(bvk::VkContext *const vk_context): bvk::Renderpass(vk_context)
{
}

void Forwardpass::on_update(u32 frame_index, u32 image_index)
{
}

void Forwardpass::on_render(
    vk::CommandBuffer const cmd,
    u32 const frame_index,
    u32 const image_index
)
{
	auto const &surface = vk_context->get_surface();

	this->cmd = cmd;
	scene = any_cast<Scene *>(user_data);
	current_pipeline = vk::Pipeline {};

	render_static_meshes();
	render_skyboxes();
}

void Forwardpass::render_static_meshes()
{
	auto const static_meshes = scene->view<StaticMeshRendererComponent const>();
	static_meshes.each([this](auto const &static_mesh) { render_static_mesh(static_mesh); });
}

void Forwardpass::render_skyboxes()
{
	auto const skyboxes = scene->view<const SkyboxComponent>();
	skyboxes.each([this](auto const &skybox) { render_skybox(skybox); });
}

void Forwardpass::render_static_mesh(StaticMeshRendererComponent const &static_mesh)
{
	auto const new_pipeline = static_mesh.material->get_effect()->get_pipeline();

	if (current_pipeline != new_pipeline)
		switch_pipeline(new_pipeline);

	draw_model(static_mesh.model);
}

void Forwardpass::render_skybox(SkyboxComponent const &skybox)
{
	auto const new_pipeline = skybox.material->get_effect()->get_pipeline();

	if (current_pipeline != new_pipeline)
		switch_pipeline(new_pipeline);

	draw_model(skybox.model);
}

void Forwardpass::switch_pipeline(vk::Pipeline pipeline)
{
	auto const extent = vk_context->get_surface().get_framebuffer_extent();
	auto const [width, height] = extent;

	current_pipeline = pipeline;
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, current_pipeline);

	cmd.setScissor(0, vk::Rect2D { { 0, 0 }, extent });
	cmd.setViewport(0, vk::Viewport { 0.0f, 0.0f, f32(width), f32(height), 0.0f, 1.0f });
}

void Forwardpass::draw_model(bvk::Model const *const model)
{
	auto const offset = VkDeviceSize { 0 };

	cmd.bindVertexBuffers(0, 1, model->get_vertex_buffer()->get_buffer(), &offset);
	cmd.bindIndexBuffer(*(model->get_index_buffer()->get_buffer()), 0u, vk::IndexType::eUint32);

	for (auto const *node : model->get_nodes())
		for (auto const &primitive : node->mesh)
		{
			auto const &material_parameters = model->get_material_parameters();
			auto const texture_index = material_parameters[primitive.material_index]
			                               .albedo_texture_index;

			cmd.drawIndexed(primitive.index_count, 1, primitive.first_index, 0, texture_index);
		}
}

#include "Rendergraphs/Passes/Forward.hpp"

Forwardpass::Forwardpass(bvk::VkContext const *const vk_context): bvk::Renderpass(vk_context)
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
	auto i = u32 { 0 };
	auto const static_meshes = scene->view<StaticMeshRendererComponent const>();

	static_meshes.each([this, &i](auto const &static_mesh) { render_static_mesh(static_mesh, i); });
}

void Forwardpass::render_skyboxes()
{
	auto const skyboxes = scene->view<const SkyboxComponent>();
	skyboxes.each([this](auto const &skybox) { render_skybox(skybox); });
}

void Forwardpass::render_static_mesh(StaticMeshRendererComponent const &static_mesh, u32 &index)
{
	auto const new_pipeline = static_mesh.material->get_shader_pipeline()->get_pipeline();

	if (current_pipeline != new_pipeline)
		switch_pipeline(new_pipeline);

	draw_model(static_mesh.model, index);
}

void Forwardpass::render_skybox(SkyboxComponent const &skybox)
{
	auto const new_pipeline = skybox.material->get_shader_pipeline()->get_pipeline();

	if (current_pipeline != new_pipeline)
		switch_pipeline(new_pipeline);

	u32 a = 0;
	draw_model(skybox.model, a);
}

void Forwardpass::switch_pipeline(vk::Pipeline pipeline)
{
	auto const extent = vk_context->get_surface()->get_framebuffer_extent();
	auto const [width, height] = extent;

	current_pipeline = pipeline;
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, current_pipeline);

	cmd.setScissor(
	    0,
	    {
	        {

	            { 0, 0 },
	            extent,
	        },
	    }
	);
	cmd.setViewport(
	    0,
	    {
	        {
	            0.0f,
	            0.0f,
	            f32(width),
	            f32(height),
	            0.0f,
	            1.0f,
	        },
	    }
	);
}

void Forwardpass::draw_model(bvk::Model const *const model, u32 &primitive_index)
{
	auto const offset = VkDeviceSize { 0 };

	cmd.bindIndexBuffer(*(model->get_index_buffer()->vk()), 0u, vk::IndexType::eUint32);

	for (auto const *node : model->get_nodes())
		for (auto const &primitive : node->mesh)
			cmd.drawIndexed(
			    primitive.index_count,
			    1,
			    primitive.first_index,
			    model->get_vertex_offset(),
			    primitive_index++
			);
}

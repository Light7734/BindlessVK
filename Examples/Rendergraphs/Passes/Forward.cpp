#include "Rendergraphs/Passes/Forward.hpp"

#include "Rendergraphs/Graph.hpp"

#include <BindlessVk/Renderer/Rendergraph.hpp>
#include <imgui.h>

Forwardpass::Forwardpass(bvk::VkContext const *const vk_context)
    : bvk::RenderNode(vk_context)
    , device(vk_context->get_device())
{
}

Forwardpass ::Forwardpass(Forwardpass &&other)
{
	*this = std::move(other);
}

Forwardpass &Forwardpass ::operator=(Forwardpass &&other)
{
	bvk::RenderNode::operator=(std::move(other));

	this->memory_allocator = other.memory_allocator;
	this->scene = other.scene;
	this->device = other.device;
	this->draw_indirect_buffer = other.draw_indirect_buffer;
	this->current_pipeline = other.current_pipeline;
	this->cmd = other.cmd;
	this->static_mesh_count = other.static_mesh_count;
	this->cull_pipeline = other.cull_pipeline;
	this->model_pipeline = other.model_pipeline;
	this->skybox_pipeline = other.skybox_pipeline;
	this->primitive_count = other.primitive_count;
	this->freeze_cull = other.freeze_cull;

	return *this;
}

void Forwardpass::on_setup(bvk::RenderNode *parent)
{
	auto const data = any_cast<UserData *>(user_data);

	scene = data->scene;
	memory_allocator = data->memory_allocator;

	draw_indirect_buffer = &parent->get_buffer_inputs().at(BasicRendergraph::U_DrawIndirect::key);

	cull_pipeline = data->cull_pipeline;
	model_pipeline = data->model_pipeline;
	skybox_pipeline = data->skybox_pipeline;

	scene->view<StaticMeshRendererComponent const>().each(
	    [this](StaticMeshRendererComponent const &mesh) {
		    auto model = mesh.model;

		    for (auto const *node : model->get_nodes())
			    primitive_count += node->mesh.size();
	    }
	);

	auto *scene_buffer = &parent->get_buffer_inputs().at(BasicRendergraph::U_SceneData::key);

	{
		auto *map = (BasicRendergraph::U_SceneData *)scene_buffer->map_block(0);
		*map = {
			{},
			primitive_count,
		};
		scene_buffer->unmap();
	}

	{
		auto *map = (BasicRendergraph::U_SceneData *)scene_buffer->map_block(1);
		*map = {
			{},
			primitive_count,
		};
		scene_buffer->unmap();
	}

	{
		auto *map = (BasicRendergraph::U_SceneData *)scene_buffer->map_block(2);
		*map = {
			{},
			primitive_count,
		};
		scene_buffer->unmap();
	}
}

void Forwardpass::on_frame_prepare(u32 frame_index, u32 image_index)
{
	static_mesh_count = scene->view<StaticMeshRendererComponent>().size();
}

void Forwardpass::on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
	ImGui::Begin("Forwardpass options");

	auto dispatch_x = 1 + (static_mesh_count / 64);
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, cull_pipeline->get_pipeline());

	ImGui::Checkbox("freeze cull", &freeze_cull);
	if (!freeze_cull)
		cmd.dispatch(1, 1, 1);

	ImGui::End();
}

void Forwardpass::on_frame_graphics(
    vk::CommandBuffer const cmd,
    u32 const frame_index,
    u32 const image_index
)
{
	auto const &surface = vk_context->get_surface();

	this->cmd = cmd;
	current_pipeline = vk::Pipeline {};

	render_static_meshes(frame_index);
	render_skyboxes();
}

void Forwardpass::render_static_meshes(u32 frame_index)
{
	switch_pipeline(model_pipeline->get_pipeline());

	cmd.drawIndexedIndirect(
	    *draw_indirect_buffer->vk(),
	    0,
	    primitive_count,
	    sizeof(BasicRendergraph::U_DrawIndirect)
	);
}

void Forwardpass::render_skyboxes()
{
	switch_pipeline(skybox_pipeline->get_pipeline());

	auto const skyboxes = scene->view<const SkyboxComponent>();
	skyboxes.each([this](auto const &skybox) { render_skybox(skybox); });
}

void Forwardpass::render_static_mesh(StaticMeshRendererComponent const &static_mesh, u32 &index)
{
	draw_model(static_mesh.model, index);
}

void Forwardpass::render_skybox(SkyboxComponent const &skybox)
{
	u32 primitive_index = 0;
	draw_model(skybox.model, primitive_index);
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
	auto const index_offset = model->get_index_offset();
	auto const vertex_offset = model->get_vertex_offset();

	for (auto const *node : model->get_nodes())
		for (auto const &primitive : node->mesh)
			cmd.drawIndexed(
			    primitive.index_count,
			    1,
			    primitive.first_index + index_offset,
			    vertex_offset,
			    primitive_index++
			);
}

#include "BindlessVk/Renderer/Rendergraph.hpp"

#include "BindlessVk/Texture/Texture.hpp"

#include <fmt/format.h>
#include <ranges>

namespace BINDLESSVK_NAMESPACE {

Rendergraph::Rendergraph(VkContext const *const vk_context): vk_context(vk_context)
{
}

Rendergraph::Rendergraph(Rendergraph &&other)
{
	*this = std::move(other);
}

Rendergraph &Rendergraph::operator=(Rendergraph &&other)
{
	this->vk_context = other.vk_context;
	this->resources = other.resources;
	this->user_data = other.user_data;
	this->passes = std::move(other.passes);
	this->buffer_inputs = std::move(other.buffer_inputs);

	this->compute_pipeline_layout = other.compute_pipeline_layout;
	this->compute_descriptor_sets = std::move(other.compute_descriptor_sets);

	this->compute_pipeline_layout = other.compute_pipeline_layout;
	this->compute_descriptor_set_layout = other.compute_descriptor_set_layout;
	this->compute_descriptor_sets = std::move(other.compute_descriptor_sets);

	this->graphics_pipeline_layout = other.graphics_pipeline_layout;
	this->graphics_descriptor_set_layout = other.graphics_descriptor_set_layout;
	this->graphics_descriptor_sets = std::move(other.graphics_descriptor_sets);

	this->prepare_label = other.prepare_label;
	this->compute_label = other.compute_label;
	this->graphics_label = other.graphics_label;
	this->present_barrier_label = other.present_barrier_label;

	other.vk_context = {};

	return *this;
}

Rendergraph::~Rendergraph()
{
	if (!vk_context)
		return;

	for (auto *pass : passes)
		delete pass;
}

RenderGraphBuilder::RenderGraphBuilder(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    DescriptorAllocator *const descriptor_allocator
)
    : vk_context(vk_context)
    , device(vk_context->get_device())
    , debug_utils(vk_context->get_debug_utils())
    , memory_allocator(memory_allocator)
    , descriptor_allocator(descriptor_allocator)
{
}

auto RenderGraphBuilder::build_graph() -> Rendergraph *
{
	backbuffer_attachment_key = pass_blueprints.back().color_attachments.back().hash;
	resources->add_key_to_attachment_index(backbuffer_attachment_key, 0);

	build_graph_buffer_inputs();
	build_graph_texture_inputs();
	build_graph_input_descriptors();
	initialize_graph_input_descriptors();

	for (u32 i = pass_blueprints.size(); i-- > 0;)
	{
		auto const &blueprint = pass_blueprints[i];
		auto &pass = graph->passes[i];

		pass->name = blueprint.name;
		pass->compute = blueprint.compute;
		pass->graphics = blueprint.graphics;
		pass->sample_count = blueprint.sample_count;
		pass->prepare_label = blueprint.prepare_label;
		pass->compute_label = blueprint.compute_label;
		pass->graphics_label = blueprint.graphics_label;
		pass->barrier_label = blueprint.barrier_label;
		pass->user_data = blueprint.user_data;

		graph->compute |= blueprint.compute;
		graph->graphics |= blueprint.graphics;

		build_pass_color_attachments(pass, blueprint.color_attachments);
		build_pass_depth_attachment(pass, blueprint.depth_attachment);
		build_pass_buffer_inputs(pass, blueprint.buffer_inputs);
		build_pass_input_descriptors(pass, blueprint.buffer_inputs, blueprint.texture_inputs);
		build_pass_texture_inputs(pass, blueprint.texture_inputs);
		initialize_pass_input_descriptors(pass, blueprint);
	}

	graph->on_setup();

	for (auto &pass : graph->passes)
		pass->on_setup(graph);

	return graph;
}

void RenderGraphBuilder::build_graph_buffer_inputs()
{
	graph->buffer_inputs.reserve(blueprint_buffer_inputs.size());

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		graph->buffer_inputs[blueprint_buffer_input.key] = Buffer(
		    vk_context,
		    memory_allocator,
		    blueprint_buffer_input.usage,
		    blueprint_buffer_input.allocation_info,
		    blueprint_buffer_input.size,
		    blueprint_buffer_input.update_frequency ==
		            RenderpassBlueprint::BufferInput::UpdateFrequency::ePerFrame ?
		        max_frames_in_flight :
		        1,
		    blueprint_buffer_input.name
		);
}

void RenderGraphBuilder::build_graph_texture_inputs()
{
}

void RenderGraphBuilder::build_graph_input_descriptors()
{
	auto graphics_bindings = vec<vk::DescriptorSetLayoutBinding> {};
	auto compute_bindings = vec<vk::DescriptorSetLayoutBinding> {};

	extract_graph_buffer_descriptor_bindings(
	    blueprint_buffer_inputs,
	    &compute_bindings,
	    &graphics_bindings
	);

	extract_graph_texture_descriptor_bindings(
	    blueprint_texture_inputs,
	    &compute_bindings,
	    &graphics_bindings
	);

	build_graph_graphics_input_descriptors(graphics_bindings);
	build_graph_compute_input_descriptors(compute_bindings);
}

void RenderGraphBuilder::extract_graph_buffer_descriptor_bindings(
    vec<RenderpassBlueprint::BufferInput> const &buffer_inputs,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	for (auto const &buffer_input : buffer_inputs)
		extract_input_descriptor_bindings(
		    buffer_input.descriptor_infos,
		    out_compute_bindings,
		    out_graphics_bindings
		);
}

void RenderGraphBuilder::extract_graph_texture_descriptor_bindings(
    vec<RenderpassBlueprint::TextureInput> const &texture_inputs,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	for (auto const &texture_input : texture_inputs)
		extract_input_descriptor_bindings(
		    texture_input.descriptor_infos,
		    out_compute_bindings,
		    out_graphics_bindings
		);
}

void RenderGraphBuilder::extract_input_descriptor_bindings(
    vec<RenderpassBlueprint::DescriptorInfo> const &descriptor_infos,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	for (auto const &descriptor_info : descriptor_infos)
		if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eCompute)
			out_compute_bindings->emplace_back(descriptor_info.layout);

		else if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
			out_graphics_bindings->emplace_back(descriptor_info.layout);
}


void RenderGraphBuilder::build_graph_graphics_input_descriptors(
    vec<vk::DescriptorSetLayoutBinding> const &bindings
)
{
	auto const flags = vec<vk::DescriptorBindingFlags>(
	    bindings.size(),
	    vk::DescriptorBindingFlagBits::ePartiallyBound
	);
	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };

	graph->graphics_descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	debug_utils->set_object_name(
	    device->vk(),
	    graph->graphics_descriptor_set_layout,
	    fmt::format("graph_graphics_descriptor_set_layout")
	);

	graph->graphics_pipeline_layout =
	    device->vk().createPipelineLayout(vk::PipelineLayoutCreateInfo {
	        {},
	        graph->graphics_descriptor_set_layout,
	    });
	debug_utils->set_object_name(
	    device->vk(),
	    graph->graphics_pipeline_layout,
	    fmt::format("graph_graphics_pipeline_layout")
	);

	if (!bindings.empty())
	{
		graph->graphics_descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			graph->graphics_descriptor_sets.emplace_back(
			    descriptor_allocator,
			    graph->graphics_descriptor_set_layout
			);

			debug_utils->set_object_name(
			    device->vk(),
			    graph->graphics_descriptor_sets.back().vk(),
			    fmt::format("graph_graphics_descriptor_set_{}", i)
			);
		}
	}
}

void RenderGraphBuilder::build_graph_compute_input_descriptors(
    vec<vk::DescriptorSetLayoutBinding> const &bindings
)
{
	auto const flags = vec<vk::DescriptorBindingFlags>(
	    bindings.size(),
	    vk::DescriptorBindingFlagBits::ePartiallyBound
	);
	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };

	graph->compute_descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	debug_utils->set_object_name(
	    device->vk(),
	    graph->compute_descriptor_set_layout,
	    fmt::format("graph_compute_descriptor_set_layout")
	);

	graph->compute_pipeline_layout =
	    device->vk().createPipelineLayout(vk::PipelineLayoutCreateInfo {
	        {},
	        graph->compute_descriptor_set_layout,
	    });

	debug_utils->set_object_name(
	    device->vk(),
	    graph->compute_pipeline_layout,
	    fmt::format("graph_compute_pipeline_layout")
	);

	if (!bindings.empty())
	{
		graph->compute_descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			graph->compute_descriptor_sets.emplace_back(
			    descriptor_allocator,
			    graph->compute_descriptor_set_layout

			);
			debug_utils->set_object_name(
			    device->vk(),
			    graph->compute_descriptor_sets.back().vk(),
			    fmt::format("graph_compute_descriptor_set_{}", i)
			);
		}
	}
}

void RenderGraphBuilder::initialize_graph_input_descriptors()
{
	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};
	buffer_infos.reserve(blueprint_buffer_inputs.size() * max_frames_in_flight * 2);
	auto writes = vec<vk::WriteDescriptorSet> {};

	for (auto const blueprint_buffer : blueprint_buffer_inputs)
	{
		auto const &buffer_input = graph->buffer_inputs[blueprint_buffer.key];

		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			if (blueprint_buffer.update_frequency ==
			    RenderpassBlueprint::BufferInput::UpdateFrequency::ePerFrame)
			{
				buffer_infos.emplace_back(vk::DescriptorBufferInfo {
				    *buffer_input.vk(),
				    buffer_input.get_block_size() * i,
				    buffer_input.get_block_size(),
				});
			}
			else
			{
				buffer_infos.emplace_back(vk::DescriptorBufferInfo {
				    *buffer_input.vk(),
				    {},
				    buffer_input.get_block_size(),
				});
			}

			for (auto const &descriptor_info : blueprint_buffer.descriptor_infos)
			{
				auto &dst_descriptor_set =
				    descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics ?
				        graph->graphics_descriptor_sets[i] :
				        graph->compute_descriptor_sets[i];


				for (u32 j = 0; j < descriptor_info.layout.descriptorCount; ++j)
				{
					writes.emplace_back(
					    dst_descriptor_set.vk(),
					    descriptor_info.layout.binding,
					    j,
					    1u,
					    descriptor_info.layout.descriptorType,
					    nullptr,
					    &buffer_infos.back()
					);
				}
			}
		}
	}

	for (auto const &texture_input_info : blueprint_texture_inputs)
	{
		if (!texture_input_info.default_texture)
			continue;

		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			for (auto const &descriptor_info : texture_input_info.descriptor_infos)
			{
				auto &dst_descriptor_set =
				    descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics ?
				        graph->graphics_descriptor_sets[i] :
				        graph->compute_descriptor_sets[i];

				for (u32 j = 0; j < descriptor_info.layout.descriptorCount; ++j)
				{
					writes.emplace_back(
					    dst_descriptor_set.vk(),
					    descriptor_info.layout.binding,
					    j,
					    1,
					    descriptor_info.layout.descriptorType,
					    texture_input_info.default_texture->get_descriptor_info()
					);
				}
			}
		}
	}

	device->vk().updateDescriptorSets(writes, {});
	device->vk().waitIdle();
}

void RenderGraphBuilder::build_pass_color_attachments(
    Renderpass *const pass,
    vec<RenderpassBlueprint::Attachment> const &blueprint_attachments
)
{
	pass->attachments.resize(blueprint_attachments.size());
	u32 i = 0;
	for (auto const &blueprint_attachment : blueprint_attachments)
	{
		auto &attachment = pass->attachments[i++];
		pass->color_attachment_formats.emplace_back(blueprint_attachment.format);

		attachment.stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		attachment.access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		attachment.image_layout = vk::ImageLayout::eColorAttachmentOptimal;
		attachment.subresource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
		attachment.store_op = vk::AttachmentStoreOp::eStore;
		attachment.clear_value = blueprint_attachment.clear_value;

		auto const has_input = !!blueprint_attachment.input_hash;
		attachment.load_op = has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;

		attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

		if (attachment.resource_index == std::numeric_limits<u32>::max())
		{
			resources->create_color_attachment(blueprint_attachment, pass->sample_count);

			attachment.resource_index =
			    resources->try_get_attachment_index(blueprint_attachment.hash);

			assert_false(attachment.resource_index == std::numeric_limits<u32>::max());
		}
		else
			resources->add_key_to_attachment_index(
			    blueprint_attachment.input_hash,
			    attachment.resource_index
			);

		if (pass->is_multisampled())
		{
			attachment.transient_resolve_moode = vk::ResolveModeFlagBits::eAverage;

			attachment.transient_resource_index =
			    resources->try_get_suitable_transient_attachment_index(
			        blueprint_attachment,
			        pass->sample_count
			    );

			if (attachment.transient_resource_index == std::numeric_limits<u32>::max())
			{
				resources->create_transient_attachment(blueprint_attachment, pass->sample_count);

				attachment.transient_resource_index =
				    resources->try_get_suitable_transient_attachment_index(
				        blueprint_attachment,
				        pass->sample_count
				    );

				assert_false(
				    attachment.transient_resource_index == std::numeric_limits<u32>::max()
				);
			}
		}
	}
}

void RenderGraphBuilder::build_pass_depth_attachment(
    Renderpass *pass,
    RenderpassBlueprint::Attachment const &blueprint_attachment
)
{
	if (!blueprint_attachment)
		return;

	pass->depth_attachment_format = blueprint_attachment.format;

	auto &attachment = pass->attachments.emplace_back();

	attachment.stage_mask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	attachment.access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
	                         vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	attachment.image_layout = vk::ImageLayout::eDepthAttachmentOptimal;
	attachment.subresource_range = {
		vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1,
	};

	auto const has_input = !!blueprint_attachment.input_hash;
	attachment.load_op = has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;
	attachment.store_op = vk::AttachmentStoreOp::eStore;
	attachment.clear_value = blueprint_attachment.clear_value;

	attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

	if (attachment.resource_index == std::numeric_limits<u32>::max())
	{
		resources->create_depth_attachment(blueprint_attachment, pass->sample_count);

		attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

		assert_false(attachment.resource_index == std::numeric_limits<u32>::max());
	}

	else
		resources->add_key_to_attachment_index(
		    attachment.resource_index,
		    blueprint_attachment.input_hash
		);
}

void RenderGraphBuilder::build_pass_buffer_inputs(
    Renderpass *pass,
    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs
)
{
	pass->buffer_inputs.reserve(blueprint_buffer_inputs.size());

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		pass->buffer_inputs.emplace_back(
		    vk_context,
		    memory_allocator,
		    blueprint_buffer_input.usage,
		    blueprint_buffer_input.allocation_info,
		    blueprint_buffer_input.size,
		    blueprint_buffer_input.update_frequency ==
		            RenderpassBlueprint::BufferInput::UpdateFrequency::ePerFrame ?
		        max_frames_in_flight :
		        1,
		    blueprint_buffer_input.name
		);
}

void RenderGraphBuilder::build_pass_texture_inputs(
    Renderpass *pass,
    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
)
{
}

void RenderGraphBuilder::build_pass_input_descriptors(
    Renderpass *pass,
    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs,
    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
)
{
	{
		auto const bindings_count =
		    blueprint_buffer_inputs.size() + blueprint_texture_inputs.size();

		auto bindings = vec<vk::DescriptorSetLayoutBinding> {};
		auto flags = vec<vk::DescriptorBindingFlags> {};

		bindings.reserve(bindings_count);
		flags.reserve(bindings_count);

		for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		{
			for (auto const &descriptor_info : blueprint_buffer_input.descriptor_infos)
			{
				if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
				{
					bindings.emplace_back(descriptor_info.layout);
					flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
				}
			}
		}

		for (auto const &blueprint_texture_input : blueprint_texture_inputs)
		{
			for (auto const &descriptor_info : blueprint_texture_input.descriptor_infos)
				if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
				{
					bindings.emplace_back(descriptor_info.layout);
					flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
				}
		}
		auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };
		pass->graphics_descriptor_set_layout = device->vk().createDescriptorSetLayout({
		    {},
		    bindings,
		    &extended_info,
		});
		debug_utils->set_object_name(
		    device->vk(),
		    pass->graphics_descriptor_set_layout,
		    fmt::format("{}_descriptor_set_layout", pass->name)
		);

		auto const layouts = arr<vk::DescriptorSetLayout, 2> {
			graph->graphics_descriptor_set_layout,
			pass->graphics_descriptor_set_layout,
		};

		pass->graphics_pipeline_layout = device->vk().createPipelineLayout({ {}, layouts });
		debug_utils->set_object_name(
		    device->vk(),
		    pass->graphics_pipeline_layout,
		    fmt::format("{}_graphics_pipeline_layout", pass->name).c_str()
		);

		if (!bindings.empty())
		{
			pass->graphics_descriptor_sets.reserve(max_frames_in_flight);
			for (u32 i = i; i < max_frames_in_flight; ++i)
			{
				pass->graphics_descriptor_sets.emplace_back(
				    descriptor_allocator,
				    pass->graphics_descriptor_set_layout
				);

				debug_utils->set_object_name(
				    device->vk(),
				    pass->graphics_descriptor_sets.back().vk(),
				    fmt::format("{}_graphics_descriptor_set_{}", pass->name, i)
				);
			}
		}
	}

	{
		auto const bindings_count =
		    blueprint_buffer_inputs.size() + blueprint_texture_inputs.size();

		auto bindings = vec<vk::DescriptorSetLayoutBinding> {};
		auto flags = vec<vk::DescriptorBindingFlags> {};

		bindings.reserve(bindings_count);
		flags.reserve(bindings_count);

		for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		{
			for (auto const &descriptor_info : blueprint_buffer_input.descriptor_infos)
			{
				if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eCompute)
				{
					bindings.emplace_back(descriptor_info.layout);
					flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
				}
			}
		}

		for (auto const &blueprint_texture_input : blueprint_texture_inputs)
		{
			for (auto const &descriptor_info : blueprint_texture_input.descriptor_infos)
				if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eCompute)
				{
					bindings.emplace_back(descriptor_info.layout);
					flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
				}
		}
		auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };
		pass->compute_descriptor_set_layout = device->vk().createDescriptorSetLayout({
		    {},
		    bindings,
		    &extended_info,
		});
		debug_utils->set_object_name(
		    device->vk(),
		    pass->compute_descriptor_set_layout,
		    fmt::format("{}_descriptor_set_layout", pass->name)
		);

		auto const layouts = arr<vk::DescriptorSetLayout, 2> {
			graph->compute_descriptor_set_layout,
			pass->compute_descriptor_set_layout,
		};

		pass->compute_pipeline_layout = device->vk().createPipelineLayout({ {}, layouts });
		debug_utils->set_object_name(
		    device->vk(),
		    pass->compute_pipeline_layout,
		    fmt::format("{}_compute_pipeline_layout", pass->name).c_str()
		);

		if (!bindings.empty())
		{
			pass->compute_descriptor_sets.reserve(max_frames_in_flight);
			for (u32 i = i; i < max_frames_in_flight; ++i)
			{
				pass->compute_descriptor_sets.emplace_back(
				    descriptor_allocator,
				    pass->compute_descriptor_set_layout
				);

				debug_utils->set_object_name(
				    device->vk(),
				    pass->compute_descriptor_sets.back().vk(),
				    fmt::format("{}_compute_descriptor_set_{}", pass->name, i)
				);
			}
		}
	}
}

void RenderGraphBuilder::initialize_pass_input_descriptors(
    Renderpass *pass,
    RenderpassBlueprint const &pass_blueprint
)
{
	auto writes = vec<vk::WriteDescriptorSet> {};
	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};

	for (u32 k = 0; auto &buffer : pass->buffer_inputs)
	{
		auto const &blueprint_buffer = pass_blueprint.buffer_inputs[k++];

		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			if (blueprint_buffer.update_frequency ==
			    RenderpassBlueprint::BufferInput::UpdateFrequency::ePerFrame)
			{
				buffer_infos.emplace_back(vk::DescriptorBufferInfo {
				    *buffer.vk(),
				    buffer.get_block_size() * i,
				    buffer.get_block_size(),
				});
			}
			else
			{
				buffer_infos.emplace_back(vk::DescriptorBufferInfo {
				    *buffer.vk(),
				    0,
				    buffer.get_block_size(),
				});
			}
		}
	}

	for (u32 i = 0; i < max_frames_in_flight; ++i)
	{
		for (auto const &blueprint_buffer : pass_blueprint.buffer_inputs)
		{
			for (auto const &descriptor_info : blueprint_buffer.descriptor_infos)
			{
				auto &dst_descriptor_set =
				    descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics ?
				        pass->graphics_descriptor_sets[i] :
				        pass->compute_descriptor_sets[i];

				for (u32 j = 0; j < descriptor_info.layout.descriptorCount; ++j)
				{
					writes.emplace_back(
					    dst_descriptor_set.vk(),
					    descriptor_info.layout.binding,
					    j,
					    1u,
					    descriptor_info.layout.descriptorType,
					    nullptr,
					    &buffer_infos[blueprint_buffer.key]
					);
				}
			}
		}
	}

	for (auto const &texture_input_info : pass_blueprint.texture_inputs)
	{
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			for (auto const &descriptor_info : texture_input_info.descriptor_infos)
			{
				auto &dst_descriptor_set =
				    descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics ?
				        pass->graphics_descriptor_sets[i] :
				        pass->compute_descriptor_sets[i];

				for (u32 j = 0; j < descriptor_info.layout.descriptorCount; ++j)
				{
					writes.emplace_back(
					    dst_descriptor_set.vk(),
					    descriptor_info.layout.binding,
					    j,
					    1,
					    descriptor_info.layout.descriptorType,
					    texture_input_info.default_texture->get_descriptor_info()
					);
				}
			}
		}
	}

	device->vk().updateDescriptorSets(writes, {});
	device->vk().waitIdle();
}
void RenderGraphBuilder::build_pass_cmd_buffer_begin_infos(Renderpass *pass)
{
	pass->cmd_buffer_inheritance_rendering_info = {
		{},
		{},
		static_cast<u32>(pass->color_attachment_formats.size()),
		pass->color_attachment_formats.data(),
		pass->depth_attachment_format,
		{},
		pass->sample_count,
		{},
	};

	pass->cmd_buffer_inheritance_info = {
		{},
		{},
		{},
		{},
		{},
		{}, // _
		&pass->cmd_buffer_inheritance_rendering_info,
	};

	pass->cmd_buffer_begin_info = {
		vk::CommandBufferUsageFlagBits::eRenderPassContinue,
		&pass->cmd_buffer_inheritance_info,
	};
}

auto RenderResources::calculate_attachment_image_extent(
    RenderpassBlueprint::Attachment const &blueprint_attachment
) const -> vk::Extent3D
{
	switch (blueprint_attachment.size_type)
	{
	case Renderpass::SizeType::eSwapchainRelative:
	{
		auto const [width, height] = blueprint_attachment.size;
		return {
			static_cast<u32>(surface->get_framebuffer_extent().width * width),
			static_cast<u32>(surface->get_framebuffer_extent().height * height),
			1,
		};
	}

	case Renderpass::SizeType::eAbsolute:
	{
		auto const [width, height] = blueprint_attachment.size;
		return {
			static_cast<u32>(width),
			static_cast<u32>(height),
			1,
		};
	}

	case Renderpass::SizeType::eRelative:
	default: assert_fail("Invalid attachment size type"); return {};
	};
}

} // namespace BINDLESSVK_NAMESPACE

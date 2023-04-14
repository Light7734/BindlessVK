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
	this->pipeline_layout = other.pipeline_layout;
	this->descriptor_set_layout = other.descriptor_set_layout;
	this->descriptor_sets = std::move(other.descriptor_sets);
	this->update_label = other.update_label;
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
		pass->sample_count = blueprint.sample_count;
		pass->update_label = blueprint.update_label;
		pass->barrier_label = blueprint.barrier_label;
		pass->render_label = blueprint.render_label;
		pass->user_data = blueprint.user_data;

		build_pass_color_attachments(pass, blueprint.color_attachments);
		build_pass_depth_attachment(pass, blueprint.depth_attachment);
		build_pass_buffer_inputs(pass, blueprint.buffer_inputs);
		build_pass_input_descriptors(pass, blueprint.buffer_inputs, blueprint.texture_inputs);
		build_pass_texture_inputs(pass, blueprint.texture_inputs);
		initialize_pass_input_descriptors(pass, blueprint);
	}

	return graph;
}

void RenderGraphBuilder::build_graph_buffer_inputs()
{
	graph->buffer_inputs.reserve(blueprint_buffer_inputs.size());
	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		graph->buffer_inputs.emplace_back(
		    vk_context,
		    memory_allocator,
		    blueprint_buffer_input.type == vk::DescriptorType::eUniformBuffer ?
		        vk::BufferUsageFlagBits::eUniformBuffer :
		        vk::BufferUsageFlagBits::eStorageBuffer,
		    vma::AllocationCreateInfo {
		        vma::AllocationCreateFlagBits::eHostAccessRandom,
		        vma::MemoryUsage::eAutoPreferDevice,
		    },
		    blueprint_buffer_input.size,
		    max_frames_in_flight,
		    blueprint_buffer_input.name
		);
}

void RenderGraphBuilder::build_graph_texture_inputs()
{
}

void RenderGraphBuilder::build_graph_input_descriptors()
{
	auto const bindings_count = blueprint_buffer_inputs.size();

	auto bindings = vec<vk::DescriptorSetLayoutBinding> {};
	auto flags = vec<vk::DescriptorBindingFlags> {};

	bindings.reserve(bindings_count);
	flags.reserve(bindings_count);

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
	{
		bindings.emplace_back(
		    blueprint_buffer_input.binding,
		    blueprint_buffer_input.type,
		    blueprint_buffer_input.count,
		    blueprint_buffer_input.stage_mask
		);
		flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
	}

	for (auto const &blueprint_texture_input : blueprint_texture_inputs)
	{
		bindings.emplace_back(
		    blueprint_texture_input.binding,
		    blueprint_texture_input.type,
		    blueprint_texture_input.count,
		    blueprint_texture_input.stage_mask
		);
		flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
	}

	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };
	graph->descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	debug_utils->set_object_name(
	    device->vk(),
	    graph->descriptor_set_layout,
	    fmt::format("graph_descriptor_set_layout")
	);


	graph->pipeline_layout =
	    device->vk().createPipelineLayout({ {}, graph->descriptor_set_layout });

	debug_utils->set_object_name(
	    device->vk(),
	    graph->pipeline_layout,
	    fmt::format("graph_pipeline_layout")
	);

	if (!bindings.empty())
	{
		graph->descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			graph->descriptor_sets.emplace_back(descriptor_allocator, graph->descriptor_set_layout);

			debug_utils->set_object_name(
			    device->vk(),
			    graph->descriptor_sets.back().vk(),
			    fmt::format("graph_descriptor_set_{}", i)
			);
		}
	}
}

void RenderGraphBuilder::initialize_graph_input_descriptors()
{
	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};
	buffer_infos.reserve(blueprint_buffer_inputs.size() * max_frames_in_flight);
	auto writes = vec<vk::WriteDescriptorSet> {};

	for (u32 k = 0; auto &buffer_input : graph->buffer_inputs)
	{
		auto const &blueprint_buffer = blueprint_buffer_inputs[k++];

		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			buffer_infos.emplace_back(vk::DescriptorBufferInfo {
			    *buffer_input.vk(),
			    buffer_input.get_block_size() * i,
			    buffer_input.get_block_size(),
			});

			for (u32 j = 0; j < blueprint_buffer.count; ++j)
			{
				writes.emplace_back(
				    graph->descriptor_sets[i].vk(),
				    blueprint_buffer.binding,
				    j,
				    1u,
				    blueprint_buffer.type,
				    nullptr,
				    &buffer_infos.back()
				);
			}
		}
	}

	for (auto const &texture_input_info : blueprint_texture_inputs)
	{
		if (!texture_input_info.default_texture)
			continue;

		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			for (u32 j = 0; j < texture_input_info.count; ++j)
			{
				writes.emplace_back(
				    graph->descriptor_sets[i].vk(),
				    texture_input_info.binding,
				    j,
				    1,
				    texture_input_info.type,
				    texture_input_info.default_texture->get_descriptor_info()
				);
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

		    blueprint_buffer_input.type == vk::DescriptorType::eUniformBuffer ?
		        vk::BufferUsageFlagBits::eUniformBuffer :
		        vk::BufferUsageFlagBits::eStorageBuffer,

		    vma::AllocationCreateInfo {
		        vma::AllocationCreateFlagBits::eHostAccessRandom,
		        vma::MemoryUsage::eAutoPreferDevice,
		    },

		    blueprint_buffer_input.size,
		    max_frames_in_flight,
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
	auto const bindings_count = blueprint_buffer_inputs.size() + blueprint_texture_inputs.size();

	auto bindings = vec<vk::DescriptorSetLayoutBinding> {};
	auto flags = vec<vk::DescriptorBindingFlags> {};

	bindings.reserve(bindings_count);
	flags.reserve(bindings_count);

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
	{
		bindings.emplace_back(
		    blueprint_buffer_input.binding,
		    blueprint_buffer_input.type,
		    blueprint_buffer_input.count,
		    blueprint_buffer_input.stage_mask
		);
		flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
	}

	for (auto const &blueprint_texture_input : blueprint_texture_inputs)
	{
		bindings.emplace_back(
		    blueprint_texture_input.binding,
		    blueprint_texture_input.type,
		    blueprint_texture_input.count,
		    blueprint_texture_input.stage_mask
		);
		flags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
	}

	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };
	pass->descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	debug_utils->set_object_name(
	    device->vk(),
	    pass->descriptor_set_layout,
	    fmt::format("{}_descriptor_set_layout", pass->name)
	);

	auto const layouts = arr<vk::DescriptorSetLayout, 2> {
		graph->descriptor_set_layout,
		pass->descriptor_set_layout,
	};

	pass->pipeline_layout = device->vk().createPipelineLayout({ {}, layouts });
	debug_utils->set_object_name(
	    device->vk(),
	    pass->pipeline_layout,
	    fmt::format("{}_pipeline_layout", pass->name).c_str()
	);

	if (!bindings.empty())
	{
		pass->descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = i; i < max_frames_in_flight; ++i)
		{
			pass->descriptor_sets.emplace_back(
			    descriptor_allocator, pass->descriptor_set_layout
			);

			debug_utils->set_object_name(
			    device->vk(),
			    pass->descriptor_sets.back().vk(),
			    fmt::format("{}_descriptor_set_{}", pass->name, i)
			);
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
			buffer_infos.emplace_back(vk::DescriptorBufferInfo {
			    *buffer.vk(),
			    buffer.get_block_size() * i,
			    buffer.get_block_size(),
			});

			for (u32 j = 0; j < buffer.get_block_count(); ++j)
			{
				writes.emplace_back(
				    pass->descriptor_sets[i].vk(),
				    blueprint_buffer.binding,
				    j,
				    1u,
				    blueprint_buffer.type,
				    nullptr,
				    &buffer_infos.back()
				);
			}
		}
	}

	for (auto const &texture_input_info : pass_blueprint.texture_inputs)
	{
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			for (u32 j = 0; j < texture_input_info.count; ++j)
			{
				writes.emplace_back(
				    pass->descriptor_sets[i].vk(),
				    texture_input_info.binding,
				    j,
				    1,
				    texture_input_info.type,
				    texture_input_info.default_texture->get_descriptor_info()
				);
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

#include "BindlessVk/Renderer/Rendergraph.hpp"

#include "Amender/Amender.hpp"
#include "BindlessVk/Texture/Texture.hpp"

namespace BINDLESSVK_NAMESPACE {

RenderGraphBuilder::RenderGraphBuilder(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    DescriptorAllocator *const descriptor_allocator
)
    : vk_context(vk_context)
    , device(vk_context->get_device())
    , memory_allocator(memory_allocator)
    , descriptor_allocator(descriptor_allocator)
{
	ScopeProfiler _;
}

void RenderGraphBuilder::build_graph()
{
	ScopeProfiler _;

	auto const backbuffer_attachment_key = node_blueprints.back().color_attachments.back().hash;
	resources->add_key_to_attachment_index(backbuffer_attachment_key, 0);

	for (auto const &blueprint : node_blueprints)
		build_node(blueprint);

	for (auto const &blueprint : node_blueprints)
		setup_node(blueprint, {});
}

void RenderGraphBuilder::build_node(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	build_node_color_attachments(node_blueprint);
	build_node_depth_attachment(node_blueprint);
	build_node_buffer_inputs(node_blueprint);
	build_node_texture_inputs(node_blueprint);
	build_node_descriptors(node_blueprint);
	initialize_node_descriptors(node_blueprint);

	for (auto const &child_node_blueprint : node_blueprint.children)
		build_node(child_node_blueprint);
}

void RenderGraphBuilder::setup_node(RenderNodeBlueprint const &node_blueprint, RenderNode *parent)
{
	ScopeProfiler _;

	node_blueprint.derived_object->on_setup(parent);

	for (auto const &child_node_blueprint : node_blueprint.children)
		setup_node(child_node_blueprint, node_blueprint.derived_object);
}

void RenderGraphBuilder::build_node_color_attachments(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	if (!node_blueprint.has_color_attachment())
		return;

	auto *const node = node_blueprint.derived_object;
	node->attachments.resize(node_blueprint.color_attachments.size());

	for (u32 i = 0; i < node->attachments.size(); ++i)
	{
		auto &attachment = node->attachments[i];
		auto const &attachment_blueprint = node_blueprint.color_attachments[i];

		attachment = create_color_attachment(attachment_blueprint, node->sample_count);
	}
}

void RenderGraphBuilder::build_node_depth_attachment(RenderNodeBlueprint const &node_blueprint)
{
	if (!node_blueprint.has_depth_attachment())
		return;

	auto *const node = node_blueprint.derived_object;
	auto &attachment = node->attachments.emplace_back();

	attachment = create_depth_attachment(node_blueprint.depth_attachment, node->sample_count);
}

void RenderGraphBuilder::build_node_buffer_inputs(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	auto *node = node_blueprint.derived_object;
	node->buffer_inputs.reserve(node_blueprint.buffer_inputs.size());

	for (auto const &buffer_blueprint : node_blueprint.buffer_inputs)
		node->buffer_inputs[buffer_blueprint.key] = Buffer(
		    vk_context,
		    memory_allocator,
		    buffer_blueprint.usage,
		    buffer_blueprint.allocation_info,
		    buffer_blueprint.size,
		    buffer_blueprint.update_frequency
		            == RenderNodeBlueprint::BufferInput::UpdateFrequency::ePerFrame ?
		        max_frames_in_flight :
		        1,
		    buffer_blueprint.name
		);
}

void RenderGraphBuilder::build_node_texture_inputs(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	log_wrn("Texture inputs are not supported (yet)");
}

void RenderGraphBuilder::build_node_descriptors(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	auto *const node = node_blueprint.derived_object;
	auto graphics_bindings = vec<vk::DescriptorSetLayoutBinding> {};
	auto compute_bindings = vec<vk::DescriptorSetLayoutBinding> {};

	extract_node_buffer_descriptor_bindings(
	    node_blueprint.buffer_inputs,
	    &compute_bindings,
	    &graphics_bindings
	);

	extract_node_texture_descriptor_bindings(
	    node_blueprint.texture_inputs,
	    &compute_bindings,
	    &graphics_bindings
	);

	build_node_graphics_descriptors(node, graphics_bindings);
	build_node_compute_descriptors(node, compute_bindings);
}

void RenderGraphBuilder::initialize_node_descriptors(RenderNodeBlueprint const &node_blueprint)
{
	ScopeProfiler _;

	auto *node = node_blueprint.derived_object;

	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};
	auto writes = vec<vk::WriteDescriptorSet> {};

	buffer_infos.reserve(node_blueprint.buffer_inputs.size() * max_frames_in_flight * 2);

	extract_node_buffer_descriptor_writes(node_blueprint, &buffer_infos, &writes);
	extract_node_texture_descriptor_writes(node_blueprint, &writes);

	device->vk().updateDescriptorSets(writes, {});
	device->vk().waitIdle();
}

auto RenderGraphBuilder::create_color_attachment(
    RenderNodeBlueprint::Attachment const &attachment_blueprint,
    vk::SampleCountFlagBits sample_count
) -> RenderNode::Attachment
{
	ScopeProfiler _;

	auto const has_input = !!attachment_blueprint.input_hash;
	auto const is_multisampled = sample_count != vk::SampleCountFlagBits::e1;

	auto attachment = RenderNode::Attachment {
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
		attachment_blueprint.clear_value,
		has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		std::numeric_limits<u32>::max(),
		std::numeric_limits<u32>::max(),
		is_multisampled ? vk::ResolveModeFlagBits::eAverage : vk::ResolveModeFlagBits::eNone
	};

	attachment.resource_index = get_or_create_color_resource(attachment_blueprint, sample_count);

	if (is_multisampled)
		attachment.transient_resource_index = get_or_create_transient_color_resource(
		    attachment_blueprint,
		    sample_count
		);

	assert_true(attachment.has_resource());
	assert_true(is_multisampled == attachment.has_transient_resource());

	return attachment;
}

auto RenderGraphBuilder::create_depth_attachment(
    RenderNodeBlueprint::Attachment const &attachment_blueprint,
    vk::SampleCountFlagBits sample_count
) -> RenderNode::Attachment
{
	ScopeProfiler _;

	auto const has_input = !!attachment_blueprint.input_hash;

	auto attachment = RenderNode::Attachment {
		vk::PipelineStageFlagBits::eEarlyFragmentTests,
		vk::AccessFlagBits::eDepthStencilAttachmentRead
		    | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
		    0,
		    1,
		    0,
		    1,
		},
		attachment_blueprint.clear_value,
		has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		std::numeric_limits<u32>::max(),
		std::numeric_limits<u32>::max(),
		vk::ResolveModeFlagBits::eNone
	};

	attachment.resource_index = get_or_create_depth_resource(attachment_blueprint, sample_count);

	assert_true(attachment.has_resource());
	assert_false(attachment.has_transient_resource());

	return attachment;
}

auto RenderGraphBuilder::get_or_create_color_resource(
    RenderNodeBlueprint::Attachment const &attachment_blueprint,
    vk::SampleCountFlagBits sample_count
) -> u32
{
	ScopeProfiler _;

	auto index = resources->try_get_attachment_index(attachment_blueprint.hash);

	if (index != std::numeric_limits<u32>::max())
		resources->add_key_to_attachment_index(attachment_blueprint.input_hash, index);
	else
	{
		resources->create_color_attachment(attachment_blueprint, sample_count);
		index = resources->try_get_attachment_index(attachment_blueprint.hash);
	}

	return index;
}

auto RenderGraphBuilder::get_or_create_transient_color_resource(
    RenderNodeBlueprint::Attachment const &attachment_blueprint,
    vk::SampleCountFlagBits const sample_count
) -> u32
{
	ScopeProfiler _;

	auto index = resources->try_get_suitable_transient_attachment_index(
	    attachment_blueprint,
	    sample_count
	);

	if (index == std::numeric_limits<u32>::max())
	{
		resources->create_transient_attachment(attachment_blueprint, sample_count);

		index = resources->try_get_suitable_transient_attachment_index(
		    attachment_blueprint,
		    sample_count
		);
	}

	return index;
}

auto RenderGraphBuilder::get_or_create_depth_resource(
    RenderNodeBlueprint::Attachment const &attachment_blueprint,
    vk::SampleCountFlagBits sample_count
) -> u32
{
	ScopeProfiler _;

	auto index = resources->try_get_attachment_index(attachment_blueprint.hash);

	if (index != std::numeric_limits<u32>::max())
		resources->add_key_to_attachment_index(attachment_blueprint.input_hash, index);
	else
	{
		resources->create_depth_attachment(attachment_blueprint, sample_count);
		index = resources->try_get_attachment_index(attachment_blueprint.hash);
	}

	return index;
}


void RenderGraphBuilder::extract_node_buffer_descriptor_bindings(
    vec<RenderNodeBlueprint::BufferInput> const &buffer_inputs,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	ScopeProfiler _;

	for (auto const &buffer_input : buffer_inputs)
		extract_input_descriptor_bindings(
		    buffer_input.descriptor_infos,
		    out_compute_bindings,
		    out_graphics_bindings
		);
}

void RenderGraphBuilder::extract_node_texture_descriptor_bindings(
    vec<RenderNodeBlueprint::TextureInput> const &texture_inputs,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	ScopeProfiler _;

	for (auto const &texture_input : texture_inputs)
		extract_input_descriptor_bindings(
		    texture_input.descriptor_infos,
		    out_compute_bindings,
		    out_graphics_bindings
		);
}

void RenderGraphBuilder::extract_input_descriptor_bindings(
    vec<RenderNodeBlueprint::DescriptorInfo> const &descriptor_infos,
    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
)
{
	ScopeProfiler _;

	for (auto const &descriptor_info : descriptor_infos)
		if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eCompute)
			out_compute_bindings->emplace_back(descriptor_info.layout);

		else if (descriptor_info.pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
			out_graphics_bindings->emplace_back(descriptor_info.layout);
}

void RenderGraphBuilder::build_node_compute_descriptors(
    RenderNode *const node,
    vec<vk::DescriptorSetLayoutBinding> const &bindings
)
{
	ScopeProfiler _;

	auto const flags = vec<vk::DescriptorBindingFlags>(
	    bindings.size(),
	    vk::DescriptorBindingFlagBits::ePartiallyBound
	);
	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };

	// descriptor set layout
	node->compute_descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	device->set_object_name(
	    node->compute_descriptor_set_layout,
	    "graph_compute_descriptor_set_layout"
	);

	// pipeline layout
	node->compute_pipeline_layout = device->vk().createPipelineLayout(vk::PipelineLayoutCreateInfo {
	    {},
	    node->compute_descriptor_set_layout,
	});
	device->set_object_name(node->compute_pipeline_layout, "graph_compute_pipeline_layout");

	// descriptor sets
	if (!bindings.empty())
	{
		node->compute_descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			node->compute_descriptor_sets.emplace_back(
			    descriptor_allocator,
			    node->compute_descriptor_set_layout

			);
			device->set_object_name(
			    node->compute_descriptor_sets.back().vk(),
			    "graph_compute_descriptor_set_{}",
			    i
			);
		}
	}
}

void RenderGraphBuilder::build_node_graphics_descriptors(
    RenderNode *const node,
    vec<vk::DescriptorSetLayoutBinding> const &bindings
)
{
	ScopeProfiler _;

	auto const flags = vec<vk::DescriptorBindingFlags>(
	    bindings.size(),
	    vk::DescriptorBindingFlagBits::ePartiallyBound
	);
	auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { flags };

	// descriptor set layout
	node->graphics_descriptor_set_layout = device->vk().createDescriptorSetLayout({
	    {},
	    bindings,
	    &extended_info,
	});
	device->set_object_name(
	    node->graphics_descriptor_set_layout,
	    "graph_graphics_descriptor_set_layout"
	);

	// pipeline layout
	node->graphics_pipeline_layout = device->vk().createPipelineLayout(
	    vk::PipelineLayoutCreateInfo {
	        {},
	        node->graphics_descriptor_set_layout,
	    }
	);
	device->set_object_name(node->graphics_pipeline_layout, "graph_graphics_pipeline_layout");

	// descriptor sets
	if (!bindings.empty())
	{
		node->graphics_descriptor_sets.reserve(max_frames_in_flight);
		for (u32 i = 0; i < max_frames_in_flight; ++i)
		{
			node->graphics_descriptor_sets.emplace_back(
			    descriptor_allocator,
			    node->graphics_descriptor_set_layout
			);

			device->set_object_name(
			    node->graphics_descriptor_sets.back().vk(),
			    "graph_graphics_descriptor_set_{}",
			    i
			);
		}
	}
}

void RenderGraphBuilder::extract_node_buffer_descriptor_writes(
    RenderNodeBlueprint const &node_blueprint,
    vec<vk::DescriptorBufferInfo> *const out_buffer_infos,
    vec<vk::WriteDescriptorSet> *out_writes
)
{
	ScopeProfiler _;

	for (auto const buffer_blueprint : node_blueprint.buffer_inputs)
		for (u32 i = 0; i < max_frames_in_flight; ++i)
			extract_frame_buffer_descriptor_writes(
			    node_blueprint.derived_object,
			    buffer_blueprint,
			    i,
			    out_buffer_infos,
			    out_writes
			);
}

void RenderGraphBuilder::extract_node_texture_descriptor_writes(
    RenderNodeBlueprint const &node_blueprint,
    vec<vk::WriteDescriptorSet> *const out_writes
)
{
	auto *const node = node_blueprint.derived_object;

	for (auto const &texture_blueprint : node_blueprint.texture_inputs)
	{
		if (!texture_blueprint.default_texture)
			continue;

		for (u32 i = 0; i < max_frames_in_flight; ++i)
			extract_frame_texture_descriptor_writes(
			    node_blueprint.derived_object,
			    texture_blueprint,
			    i,
			    out_writes
			);
	}
}

void RenderGraphBuilder::extract_frame_buffer_descriptor_writes(
    RenderNode *const node,
    RenderNodeBlueprint::BufferInput const &buffer_blueprint,
    u32 frame_index,
    vec<vk::DescriptorBufferInfo> *out_buffer_infos,
    vec<vk::WriteDescriptorSet> *out_writes
)
{
	ScopeProfiler _;

	auto const per_frame = buffer_blueprint.update_frequency
	                       == RenderNodeBlueprint::BufferInput::UpdateFrequency::ePerFrame;

	auto const &buffer_input = node->buffer_inputs[buffer_blueprint.key];

	out_buffer_infos->emplace_back(vk::DescriptorBufferInfo {
	    *buffer_input.vk(),
	    per_frame ? buffer_input.get_block_size() * frame_index : 0,
	    buffer_input.get_block_size(),
	});

	for (auto const &descriptor_info : buffer_blueprint.descriptor_infos)
		extract_descriptor_writes(
		    descriptor_info,
		    node->get_descriptor_set(descriptor_info.pipeline_bind_point, frame_index).vk(),
		    {},
		    &out_buffer_infos->back(),
		    out_writes
		);
}

void RenderGraphBuilder::extract_frame_texture_descriptor_writes(
    RenderNode *node,
    RenderNodeBlueprint::TextureInput const &texture_blueprint,
    u32 frame_index,
    vec<vk::WriteDescriptorSet> *out_writes
)
{
	ScopeProfiler _;

	for (auto const &descriptor_info : texture_blueprint.descriptor_infos)
		extract_descriptor_writes(
		    descriptor_info,
		    node->get_descriptor_set(descriptor_info.pipeline_bind_point, frame_index).vk(),
		    texture_blueprint.default_texture->get_descriptor_info(),
		    {},
		    out_writes
		);
}


void RenderGraphBuilder::extract_descriptor_writes(
    RenderNodeBlueprint::DescriptorInfo const &descriptor_info,
    vk::DescriptorSet const descriptor_set,
    vk::DescriptorImageInfo const *const image_info,
    vk::DescriptorBufferInfo const *const buffer_info,
    vec<vk::WriteDescriptorSet> *const out_writes
)
{
	ScopeProfiler _;

	for (u32 i = 0; i < descriptor_info.layout.descriptorCount; ++i)
		out_writes->emplace_back(
		    descriptor_set,
		    descriptor_info.layout.binding,
		    i,
		    1u,
		    descriptor_info.layout.descriptorType,
		    image_info,
		    buffer_info
		);
}

} // namespace BINDLESSVK_NAMESPACE

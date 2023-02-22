#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/RenderResources.hpp"
#include "BindlessVk/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

struct RenderGraph
{
	using UpdateFn = void (*)(VkContext *, RenderGraph *, u32, void *);

	class RenderResources *resources;

	vec<Renderpass> passes;

	UpdateFn on_update;
	vec<Buffer> buffer_inputs;

	vk::PipelineLayout pipeline_layout;
	vk::DescriptorSetLayout descriptor_set_layout;
	vec<AllocatedDescriptorSet> descriptor_sets;

	vk::SampleCountFlagBits sample_count;

	vk::DebugUtilsLabelEXT update_label;
	vk::DebugUtilsLabelEXT present_barriers_label;
};

/**
 * @todo use vk_context->set_object_name
 * @todo: Validate graph before build
 * @todo: Re-order passes before build
 * @todo: Texture inputs
 */
class RenderGraphBuilder
{
public:
	RenderGraphBuilder(VkContext *vk_context): vk_context(vk_context)
	{
	}

	auto build() -> RenderGraph;

	inline auto set_resources(RenderResources *resources) -> RenderGraphBuilder &
	{
		this->resources = resources;
		return *this;
	}

	inline auto add_pass(RenderpassBlueprint blueprint) -> RenderGraphBuilder &
	{
		this->pass_blueprints.push_back(blueprint);
		return *this;
	}

	inline auto add_buffer_input(RenderpassBlueprint::BufferInput const buffer_input_info)
	    -> RenderGraphBuilder &

	{
		this->blueprint_buffer_inputs.push_back(buffer_input_info);
		return *this;
	}

	inline auto set_update_fn(void (*fn)(VkContext *, RenderGraph *, u32, void *))
	    -> RenderGraphBuilder &
	{
		this->graph.on_update = fn;
		return *this;
	}

	inline auto set_update_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph.update_label = label;
		return *this;
	}

	inline auto set_present_barriers_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph.present_barriers_label = label;
		return *this;
	}

private:
	void build_graph_buffer_inputs();
	void build_graph_texture_inputs();
	void build_graph_input_descriptors();
	void initialize_graph_input_descriptors();

	void build_pass_color_attachments(
	    Renderpass &pass,
	    vec<RenderpassBlueprint::Attachment> const &blueprint_attachments
	);

	void build_pass_depth_attachment(
	    Renderpass &pass,
	    RenderpassBlueprint::Attachment const &blueprint_attachment
	);

	void build_pass_buffer_inputs(
	    Renderpass &pass,
	    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs
	);

	void build_pass_texture_inputs(
	    Renderpass &pass,
	    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
	);

	void build_pass_input_descriptors(
	    Renderpass &pass,
	    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs,
	    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
	);

	void build_pass_cmd_buffer_begin_infos(Renderpass &pass);


	void initialize_pass_input_descriptors(
	    Renderpass &pass,
	    RenderpassBlueprint const &pass_blueprint
	);

private:
	VkContext *const vk_context = {};
	RenderResources *resources = {};

	RenderGraph graph = {};

	u64 backbuffer_attachment_key = {};

	vec<RenderpassBlueprint> pass_blueprints {};
	vec<RenderpassBlueprint::BufferInput> blueprint_buffer_inputs = {};
};

} // namespace BINDLESSVK_NAMESPACE

#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/RenderResources.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"

namespace BINDLESSVK_NAMESPACE {

class Rendergraph
{
public:
	friend class RenderGraphBuilder;

public:
	Rendergraph() = default;
	Rendergraph(VkContext const *vk_context);

	Rendergraph(Rendergraph &&other);
	Rendergraph &operator=(Rendergraph &&other);

	Rendergraph(Rendergraph const &) = delete;
	Rendergraph &operator=(Rendergraph const &) = delete;

	virtual ~Rendergraph();

	void virtual on_update(u32 frame_index, u32 image_index) = 0;

	auto &get_passes() const
	{
		return passes;
	}

	auto &get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	auto &get_descriptor_sets() const
	{
		return descriptor_sets;
	}

	auto &get_update_label() const
	{
		return update_label;
	}

	auto &get_present_barrier_label() const
	{
		return present_barrier_label;
	}

protected:
	VkContext const *vk_context;

	class RenderResources *resources;

	std::any user_data;

	vec<Renderpass *> passes;

	vec<Buffer> buffer_inputs;

	vk::PipelineLayout pipeline_layout;
	vk::DescriptorSetLayout descriptor_set_layout;
	vec<AllocatedDescriptorSet> descriptor_sets;

	vk::DebugUtilsLabelEXT update_label;
	vk::DebugUtilsLabelEXT present_barrier_label;
};

class RenderGraphBuilder
{
public:
	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 * @param memory_allocator The memory allocator
	 * @param descriptor_allocator The descriptor allocator
	 */
	RenderGraphBuilder(
	    VkContext const *vk_context,
	    MemoryAllocator const *memory_allocator,
	    DescriptorAllocator *descriptor_allocator
	);

	/** Build the rendergraph */
	auto build_graph() -> Rendergraph *;

	/** Sets the rendergraph's derived type */
	template<typename T>
	auto set_type() -> RenderGraphBuilder &
	{
		graph = new T(vk_context);
		return *this;
	}

	/** Adds a renderpass to the rendergraph */
	template<typename T>
	auto add_pass(RenderpassBlueprint blueprint) -> RenderGraphBuilder &
	{
		this->pass_blueprints.push_back(blueprint);
		this->graph->passes.push_back(new T(vk_context));
		return *this;
	}

	auto set_resources(RenderResources *resources) -> RenderGraphBuilder &
	{
		this->resources = resources;
		return *this;
	}

	auto set_user_data(std::any data) -> RenderGraphBuilder &
	{
		this->graph->user_data = data;
		return *this;
	}

	auto add_buffer_input(RenderpassBlueprint::BufferInput const buffer_input_info)
	    -> RenderGraphBuilder &

	{
		this->blueprint_buffer_inputs.push_back(buffer_input_info);
		return *this;
	}

	auto add_texture_input(RenderpassBlueprint::TextureInput const input) -> RenderGraphBuilder &
	{
		this->blueprint_texture_inputs.push_back(input);
		return *this;
	}

	auto set_update_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph->update_label = label;
		return *this;
	}

	auto set_present_barrier_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph->present_barrier_label = label;
		return *this;
	}

private:
	void build_graph_buffer_inputs();
	void build_graph_texture_inputs();
	void build_graph_input_descriptors();
	void initialize_graph_input_descriptors();

	void build_pass_color_attachments(
	    Renderpass *pass,
	    vec<RenderpassBlueprint::Attachment> const &blueprint_attachments
	);

	void build_pass_depth_attachment(
	    Renderpass *pass,
	    RenderpassBlueprint::Attachment const &blueprint_attachment
	);

	void build_pass_buffer_inputs(
	    Renderpass *pass,
	    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs
	);

	void build_pass_texture_inputs(
	    Renderpass *pass,
	    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
	);

	void build_pass_input_descriptors(
	    Renderpass *pass,
	    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs,
	    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
	);

	void build_pass_cmd_buffer_begin_infos(Renderpass *pass);

	void initialize_pass_input_descriptors(
	    Renderpass *pass,
	    RenderpassBlueprint const &pass_blueprint
	);

private:
	VkContext const *vk_context = {};
	Device const *device = {};
	DebugUtils const *debug_utils = {};
	MemoryAllocator const *memory_allocator = {};
	DescriptorAllocator *descriptor_allocator = {};

	Rendergraph *graph = {};

	RenderResources *resources = {};

	u64 backbuffer_attachment_key = {};

	vec<RenderpassBlueprint> pass_blueprints {};

	vec<RenderpassBlueprint::BufferInput> blueprint_buffer_inputs = {};
	vec<RenderpassBlueprint::TextureInput> blueprint_texture_inputs = {};
};

} // namespace BINDLESSVK_NAMESPACE

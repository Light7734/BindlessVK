#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/RenderResources.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Represents the rendering passes and output of a frame */
class Rendergraph
{
public:
	friend class RenderGraphBuilder;


public:
	/** Default constructor */
	Rendergraph() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	Rendergraph(VkContext const *vk_context);

	/** Move constructor */
	Rendergraph(Rendergraph &&other);

	/** Move assignment operato */
	Rendergraph &operator=(Rendergraph &&other);

	/** Copy constructor */
	Rendergraph(Rendergraph const &) = delete;

	/** Deleted copy assignment operato */
	Rendergraph &operator=(Rendergraph const &) = delete;

	/** Destructor */
	virtual ~Rendergraph();

	/**  Sets up the graph, called only once */
	void virtual on_setup() = 0;

	/** Prepares the graph for rendering, (eg. update global descriptor set)*/
	void virtual on_frame_prepare(u32 frame_index, u32 image_index) = 0;

	/** Prepares the graph for compute operations */
	void virtual on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) = 0;

	/** Prepares the graph for compute operations */
	void virtual on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) = 0;

	/** Tirvial accessor for compute */
	auto has_compute() const
	{
		return compute;
	}

	/** Tirvial accessor for compute */
	auto has_graphics() const
	{
		return graphics;
	}

	/** Tirvial reference-accessor for passes */
	auto &get_passes() const
	{
		return passes;
	}

	/** Trivial address-accessor for @a buffer_inputs[key] */
	auto get_buffer(usize key)
	{
		return &buffer_inputs.at(key);
	}

	/** Trivial reference-accessor for compute_pipeline_layout */
	auto &get_compute_pipeline_layout() const
	{
		return compute_pipeline_layout;
	}

	/** Trivial reference-accessor for compute_descriptor_sets */
	auto &get_compute_descriptor_sets() const
	{
		return compute_descriptor_sets;
	}

	/** Trivial reference-accessor for pipeline_layout */
	auto &get_pipeline_layout() const
	{
		return graphics_pipeline_layout;
	}

	/** Trivial referrence-accessor for descripto_sets */
	auto &get_descriptor_sets() const
	{
		return graphics_descriptor_sets;
	}

	auto &get_prepare_label() const
	{
		return prepare_label;
	}

	/** Trivial referrence-accessor for compute_label */
	auto &get_compute_label() const
	{
		return compute_label;
	}

	/** Trivial referrence-accessor for update_label */
	auto &get_graphics_label() const
	{
		return graphics_label;
	}

	/** Trivial referrence-accessor for present_barrier_label */
	auto &get_present_barrier_label() const
	{
		return present_barrier_label;
	}

protected:
	VkContext const *vk_context = {};

	class RenderResources *resources = {};

	bool compute = {};
	bool graphics = {};

	std::any user_data = {};

	vec<Renderpass *> passes = {};

	hash_map<usize, Buffer> buffer_inputs = {};

	vk::PipelineLayout compute_pipeline_layout = {};
	vk::DescriptorSetLayout compute_descriptor_set_layout = {};
	vec<DescriptorSet> compute_descriptor_sets = {};

	vk::PipelineLayout graphics_pipeline_layout = {};
	vk::DescriptorSetLayout graphics_descriptor_set_layout = {};
	vec<DescriptorSet> graphics_descriptor_sets = {};

	vk::DebugUtilsLabelEXT prepare_label = {};
	vk::DebugUtilsLabelEXT compute_label = {};
	vk::DebugUtilsLabelEXT graphics_label = {};
	vk::DebugUtilsLabelEXT present_barrier_label = {};
};

/** A builder class for building render graphs */
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

	/** Sets the pointer to render resources */
	auto set_resources(RenderResources *resources) -> RenderGraphBuilder &
	{
		this->resources = resources;
		return *this;
	}

	/** Sets the user-defined data to be accessed in graph's on_update */
	auto set_user_data(std::any data) -> RenderGraphBuilder &
	{
		this->graph->user_data = data;
		return *this;
	}

	/** Adds a buffer input to gaph */
	auto add_buffer_input(RenderpassBlueprint::BufferInput const buffer_input_info)
	    -> RenderGraphBuilder &

	{
		this->blueprint_buffer_inputs.push_back(buffer_input_info);
		return *this;
	}

	/** Adds a textuer input to graph */
	auto add_texture_input(RenderpassBlueprint::TextureInput const input) -> RenderGraphBuilder &
	{
		this->blueprint_texture_inputs.push_back(input);
		return *this;
	}

	/** Sets the prepare label */
	auto set_prepare_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph->prepare_label = label;
		return *this;
	}

	/** Sets the compute label */
	auto set_compute_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph->compute_label = label;
		return *this;
	}

	/** Sets the graphics label */
	auto set_graphics_label(vk::DebugUtilsLabelEXT label) -> RenderGraphBuilder &
	{
		this->graph->graphics_label = label;
		return *this;
	}

	/** Sets the present barrier label */
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

	void build_graph_graphics_input_descriptors(vec<vk::DescriptorSetLayoutBinding> const &bindings
	);
	void build_graph_compute_input_descriptors(vec<vk::DescriptorSetLayoutBinding> const &bindings);

	void extract_graph_buffer_descriptor_bindings(
	    vec<RenderpassBlueprint::BufferInput> const &buffer_infos,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);

	void extract_graph_texture_descriptor_bindings(
	    vec<RenderpassBlueprint::TextureInput> const &texture_inputs,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);

	void extract_input_descriptor_bindings(
	    vec<RenderpassBlueprint::DescriptorInfo> const &descriptor_infos,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);

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

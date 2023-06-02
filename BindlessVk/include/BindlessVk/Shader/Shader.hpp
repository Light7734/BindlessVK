#pragma once

#include "BindlessVk/Allocators/LayoutAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan shader module that holds descriptor set slot 2's bindings */
struct Shader
{
	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	vec<vk::DescriptorSetLayoutBinding> descriptor_set_bindings;
};

/** Wrapper around vulkan graphics pipeline */
class ShaderPipeline
{
public:
	/** Pipeline state of the shader */
	struct Configuration
	{
		vk::PipelineVertexInputStateCreateInfo vertex_input_state;
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
		vk::PipelineTessellationStateCreateInfo tesselation_state;
		vk::PipelineViewportStateCreateInfo viewport_state;
		vk::PipelineRasterizationStateCreateInfo rasterization_state;
		vk::PipelineMultisampleStateCreateInfo multisample_state;
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;

		vk::PipelineColorBlendStateCreateInfo color_blend_state;

		vec<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
		vec<vk::DynamicState> dynamic_states;
	};

	enum class Type
	{
		eCompute,
		eGraphics,
	};

public:
	/** Default constructor */
	ShaderPipeline() = default;

	/** Argumented constructor */
	ShaderPipeline(
	    VkContext const *vk_context,
	    LayoutAllocator *const layout_allocator,
	    Type type,
	    vec<Shader *> const &shaders,
	    ShaderPipeline::Configuration const &configuration,
	    DescriptorSetLayoutWithHash graph_set_bindings,
	    DescriptorSetLayoutWithHash pass_set_bindings,
	    str_view debug_name = default_debug_name
	);

	/** Default move constructor */
	ShaderPipeline(ShaderPipeline &&other) = default;

	/** Default move assignment operator */
	ShaderPipeline &operator=(ShaderPipeline &&other) = default;

	/** Deleted copy constructor */
	ShaderPipeline(const ShaderPipeline &) = delete;

	/** Deleted copy assignment operator */
	ShaderPipeline &operator=(const ShaderPipeline &) = delete;

	/** Destructor */
	~ShaderPipeline();

	/** Returns null terminated str view to debug_name */
	auto get_name() const
	{
		return str_view(debug_name);
	}

	/** Trivial accessor for pipeline */
	auto get_pipeline() const
	{
		return pipeline;
	}

	/** Trivial accessor for pipeline_layout */
	auto get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	/** Trivial accessor for descriptor_set_layout
	 * @warning This will return an invalid descriptor set layout if the pipeline doesn't uses the
	 * descriptor set slot 2 (per shader descriptor set)
	 */
	auto get_descriptor_set_layout() const
	{
		return descriptor_set_layout;
	}

	/** Checks if shader pipeline uses set slot 2 (per shader descriptor set)
	 * It does so by validating descriptor_set_layout
	 */
	auto uses_shader_descriptor_set_slot() const
	{
		return !!descriptor_set_layout;
	}

private:
	void create_descriptor_set_layout(vec<Shader *> const &shaders);

	void create_pipeline_layout(
	    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
	    DescriptorSetLayoutWithHash pass_descriptor_set_layout
	);

	void create_graphics_pipeline(
	    vec<Shader *> const &shaders,
	    ShaderPipeline::Configuration configuration
	);

	void create_compute_pipeline(vec<Shader *> const &shaders);

	auto combine_descriptor_sets_bindings(vec<Shader *> const &shaders) const
	    -> vec<vk::DescriptorSetLayoutBinding>;

	auto create_shader_stage_create_infos(vec<Shader *> const &shaders) const
	    -> vec<vk::PipelineShaderStageCreateInfo>;


private:
	tidy_ptr<Device const> device = {};

	Surface const *surface = {};
	DebugUtils const *debug_utils = {};
	LayoutAllocator *layout_allocator = {};

	vk::Pipeline pipeline = {};
	vk::PipelineLayout pipeline_layout = {};
	DescriptorSetLayoutWithHash descriptor_set_layout = {};

	str debug_name = {};
};


} // namespace BINDLESSVK_NAMESPACE

#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Renderer/RenderResources.hpp"

namespace BINDLESSVK_NAMESPACE {

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

	/** Builds the rendergraph */
	void build_graph();

	/** Sets the pointer to render resources */
	auto set_resources(RenderResources *resources) -> RenderGraphBuilder &
	{
		this->resources = resources;
		return *this;
	}

	/** Pushes a render node to the graph */
	auto push_node(RenderNodeBlueprint blueprint) -> RenderGraphBuilder &
	{
		this->node_blueprints.push_back(blueprint);
		return *this;
	}

private:
	void build_node(RenderNodeBlueprint const &blueprint);
	void setup_node(RenderNodeBlueprint const &blueprint, RenderNode *parent);

	void build_node_color_attachments(RenderNodeBlueprint const &node_blueprint);
	void build_node_depth_attachment(RenderNodeBlueprint const &node_blueprint);
	void build_node_buffer_inputs(RenderNodeBlueprint const &node_blueprint);
	void build_node_texture_inputs(RenderNodeBlueprint const &node_blueprint);
	void build_node_descriptors(RenderNodeBlueprint const &node_blueprint);
	void initialize_node_descriptors(RenderNodeBlueprint const &node_blueprint);

	auto create_color_attachment(
	    RenderNodeBlueprint::Attachment const &attachment_blueprint,
	    vk::SampleCountFlagBits sample_count
	) -> RenderNode::Attachment;

	auto create_depth_attachment(
	    RenderNodeBlueprint::Attachment const &attachment_blueprint,
	    vk::SampleCountFlagBits sample_count
	) -> RenderNode::Attachment;

	auto get_or_create_color_resource(
	    RenderNodeBlueprint::Attachment const &attachment_blueprint,
	    vk::SampleCountFlagBits sample_count
	) -> u32;

	auto get_or_create_transient_color_resource(
	    RenderNodeBlueprint::Attachment const &attachment_blueprint,
	    vk::SampleCountFlagBits sample_count
	) -> u32;

	auto get_or_create_depth_resource(
	    RenderNodeBlueprint::Attachment const &attachment_blueprint,
	    vk::SampleCountFlagBits sample_count
	) -> u32;


	void build_node_compute_descriptors(
	    RenderNode *node,
	    vec<vk::DescriptorSetLayoutBinding> const &bindings
	);

	void build_node_graphics_descriptors(
	    RenderNode *node,
	    vec<vk::DescriptorSetLayoutBinding> const &bindings
	);

	void extract_node_buffer_descriptor_writes(
	    RenderNodeBlueprint const &node_blueprint,
	    vec<vk::DescriptorBufferInfo> *out_buffer_infos,
	    vec<vk::WriteDescriptorSet> *out_writes
	);

	void extract_node_texture_descriptor_writes(
	    RenderNodeBlueprint const &node_blueprint,
	    vec<vk::WriteDescriptorSet> *out_writes
	);

	void extract_frame_buffer_descriptor_writes(
	    RenderNode *node,
	    RenderNodeBlueprint::BufferInput const &buffer_blueprint,
	    u32 frame_index,
	    vec<vk::DescriptorBufferInfo> *out_buffer_infos,
	    vec<vk::WriteDescriptorSet> *out_writes
	);

	void extract_frame_texture_descriptor_writes(
	    RenderNode *node,
	    RenderNodeBlueprint::TextureInput const &texture_blueprint,
	    u32 frame_index,
	    vec<vk::WriteDescriptorSet> *out_writes
	);

	void extract_descriptor_writes(
	    RenderNodeBlueprint::DescriptorInfo const &descriptor_info,
	    vk::DescriptorSet descriptor_set,
	    vk::DescriptorImageInfo const *image_info,
	    vk::DescriptorBufferInfo const *buffer_info,
	    vec<vk::WriteDescriptorSet> *out_writes
	);

	void extract_node_buffer_descriptor_bindings(
	    vec<RenderNodeBlueprint::BufferInput> const &buffer_infos,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);

	void extract_node_texture_descriptor_bindings(
	    vec<RenderNodeBlueprint::TextureInput> const &texture_inputs,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);

	void extract_input_descriptor_bindings(
	    vec<RenderNodeBlueprint::DescriptorInfo> const &descriptor_infos,
	    vec<vk::DescriptorSetLayoutBinding> *out_compute_bindings,
	    vec<vk::DescriptorSetLayoutBinding> *out_graphics_bindings
	);


private:
	VkContext const *vk_context = {};

	Device const *device = {};

	MemoryAllocator const *memory_allocator = {};
	DescriptorAllocator *descriptor_allocator = {};

	RenderResources *resources = {};

	vec<RenderNodeBlueprint> node_blueprints {};
};

} // namespace BINDLESSVK_NAMESPACE

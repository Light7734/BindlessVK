#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan descriptor set layout providing a hash */
struct DescriptorSetLayoutWithHash
{
	vk::DescriptorSetLayout descriptor_set_layout;
	u64 hash;

	auto vk() const
	{
		return descriptor_set_layout;
	}

	operator bool() const
	{
		return !!descriptor_set_layout;
	}
};

/** Manages descriptor set and pipeline layout allocations.
 * @warning currently we don't de-allocate any layouts before destruction.
 */
class LayoutAllocator
{
public:
	/** Default constuctor */
	LayoutAllocator() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	LayoutAllocator(VkContext const *vk_context);

	/** Move constuctor */
	LayoutAllocator(LayoutAllocator &&other);

	/** Move assignment opeator */
	LayoutAllocator &operator=(LayoutAllocator &&other);

	/** Deleted copy constructor */
	LayoutAllocator(LayoutAllocator const &) = delete;

	/** Deleted copy assignment opeator */
	LayoutAllocator &operator=(LayoutAllocator const &other) = delete;

	/** Destructor */
	~LayoutAllocator();

	/** Get or create descriptor set layout. */
	auto goc_descriptor_set_layout(
	    vk::DescriptorSetLayoutCreateFlags layout_flags,
	    span<vk::DescriptorSetLayoutBinding const> bindings,
	    span<vk::DescriptorBindingFlags const> binding_flags
	) -> DescriptorSetLayoutWithHash;

	/** Get or create pipeline layout. */
	auto goc_pipeline_layout(
	    vk::PipelineLayoutCreateFlags layout_flags,
	    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
	    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
	    DescriptorSetLayoutWithHash shader_descriptor_set_layout
	) -> vk::PipelineLayout;

private:
	auto hash_descriptor_set_layout_info(
	    vk::DescriptorSetLayoutCreateFlags layout_flags,
	    span<vk::DescriptorSetLayoutBinding const> bindings,
	    span<vk::DescriptorBindingFlags const> binding_flags
	) -> u64;

	auto hash_pipeline_layout_info(
	    vk::PipelineLayoutCreateFlags layout_flags,
	    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
	    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
	    DescriptorSetLayoutWithHash shader_descriptor_set_layout
	) -> u64;

private:
	Device *device = {};

	hash_map<u64, vk::DescriptorSetLayout> descriptor_set_layouts = {};
	hash_map<u64, vk::PipelineLayout> pipeline_layouts = {};
};

} // namespace BINDLESSVK_NAMESPACE

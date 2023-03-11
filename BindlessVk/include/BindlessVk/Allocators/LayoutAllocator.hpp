#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct DescriptorSetLayoutWithHash
{
	vk::DescriptorSetLayout descriptor_set_layout;
	u64 hash;

	operator vk::DescriptorSetLayout() const
	{
		return descriptor_set_layout;
	}

	operator bool() const
	{
		return !!descriptor_set_layout;
	}
};

/** @warn currently we don't de-allocate any layouts before destruction... */
class LayoutAllocator
{
public:
	void init(vk::Device device);

	void destroy();

	/** @brief get or create descriptor set layout */
	auto goc_descriptor_set_layout(
	    vk::DescriptorSetLayoutCreateFlags layout_flags,
	    span<vk::DescriptorSetLayoutBinding const> bindings,
	    span<vk::DescriptorBindingFlags const> binding_flags
	) -> DescriptorSetLayoutWithHash;

	/** @brief get or create pipeline layout */
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
	vk::Device device = {};

	hash_map<u64, vk::DescriptorSetLayout> descriptor_set_layouts = {};
	hash_map<u64, vk::PipelineLayout> pipeline_layouts = {};
};

} // namespace BINDLESSVK_NAMESPACE

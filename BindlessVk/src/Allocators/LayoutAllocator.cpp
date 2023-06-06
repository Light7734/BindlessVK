#include "BindlessVk/Allocators/LayoutAllocator.hpp"

#include "Amender/Amender.hpp"

namespace BINDLESSVK_NAMESPACE {

LayoutAllocator::LayoutAllocator(VkContext const *const context): device(context->get_device())
{
	ScopeProfiler _;
}

LayoutAllocator::~LayoutAllocator()
{
	ScopeProfiler _;

	if (!device)
		return;

	for (auto const &[hash, layout] : descriptor_set_layouts)
		device->vk().destroyDescriptorSetLayout(layout);

	for (auto const &[hash, layout] : pipeline_layouts)
		device->vk().destroyPipelineLayout(layout);
}

auto LayoutAllocator::goc_descriptor_set_layout(
    vk::DescriptorSetLayoutCreateFlags layout_flags,
    span<vk::DescriptorSetLayoutBinding const> bindings,
    span<vk::DescriptorBindingFlags const> binding_flags
) -> DescriptorSetLayoutWithHash
{
	ScopeProfiler _;

	auto const hash = hash_descriptor_set_layout_info(layout_flags, bindings, binding_flags);

	if (!descriptor_set_layouts.contains(hash))
	{
		auto const extended_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo { binding_flags };

		descriptor_set_layouts.emplace(
		    hash,
		    device->vk().createDescriptorSetLayout({
		        layout_flags,
		        bindings,
		        &extended_info,
		    })
		);
	}

	return { descriptor_set_layouts[hash], hash };
}

auto LayoutAllocator::goc_pipeline_layout(
    vk::PipelineLayoutCreateFlags layout_flags,
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
    DescriptorSetLayoutWithHash shader_descriptor_set_layout
) -> vk::PipelineLayout
{
	ScopeProfiler _;

	auto const hash = hash_pipeline_layout_info(
	    layout_flags,
	    graph_descriptor_set_layout,
	    pass_descriptor_set_layout,
	    shader_descriptor_set_layout
	);

	if (!pipeline_layouts.contains(hash))
	{
		auto set_layouts = vec<vk::DescriptorSetLayout> {};
		set_layouts.reserve(3);

		if (graph_descriptor_set_layout)
			set_layouts.push_back(graph_descriptor_set_layout.vk());

		if (pass_descriptor_set_layout)
			set_layouts.push_back(pass_descriptor_set_layout.vk());

		if (shader_descriptor_set_layout)
			set_layouts.push_back(shader_descriptor_set_layout.vk());

		pipeline_layouts.emplace(
		    hash,
		    device->vk().createPipelineLayout(vk::PipelineLayoutCreateInfo {
		        layout_flags,
		        set_layouts,
		    })
		);
	}

	return pipeline_layouts[hash];
}


auto LayoutAllocator::hash_descriptor_set_layout_info(
    vk::DescriptorSetLayoutCreateFlags layout_flags,
    span<vk::DescriptorSetLayoutBinding const> bindings,
    span<vk::DescriptorBindingFlags const> binding_flags
) -> u64
{
	ScopeProfiler _;

	auto hash = u64 {};

	for (auto const &binding : bindings)
	{
		hash ^= hash_t(hash, binding.binding);
		hash ^= hash_t(hash, static_cast<u32>(binding.stageFlags));
		hash ^= hash_t(hash, static_cast<u32>(binding.descriptorType));
		hash ^= hash_t(hash, static_cast<u32>(binding.descriptorCount));
		assert_false(binding.pImmutableSamplers, "Immutable samplres are not supported");
	}

	for (auto const &binding_flag : binding_flags)
		hash ^= hash_t(hash, static_cast<u32>(binding_flag));

	return hash;
}

auto LayoutAllocator::hash_pipeline_layout_info(
    vk::PipelineLayoutCreateFlags layout_flags,
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
    DescriptorSetLayoutWithHash shader_descriptor_set_layout
) -> u64
{
	ScopeProfiler _;

	auto hash = u64 {};

	hash ^= hash_t(hash, static_cast<u32>(layout_flags));
	hash ^= hash_t(hash, graph_descriptor_set_layout.hash);
	hash ^= hash_t(hash, pass_descriptor_set_layout.hash);
	hash ^= hash_t(hash, shader_descriptor_set_layout.hash);

	return hash;
}


} // namespace BINDLESSVK_NAMESPACE

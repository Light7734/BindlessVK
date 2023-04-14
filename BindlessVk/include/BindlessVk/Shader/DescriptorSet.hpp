#pragma once

#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vk::DescriptorrSet */
class DescriptorSet
{
public:
	/** Default constructor */
	DescriptorSet() = default;

	/** Argumented constructor
	 *
	 * @param descriptor_allocator The descriptor allocator
	 * @param descriptor_set_layout A descriptor set layout
	 */
	DescriptorSet(
	    DescriptorAllocator *descriptor_allocator,
	    vk::DescriptorSetLayout descriptor_set_layout
	);

	/** Move constructor */
	DescriptorSet(DescriptorSet &&other);

	/** Move assignment operator */
	DescriptorSet &operator=(DescriptorSet &&);

	/** Deleted copy constructor  */
	DescriptorSet(DescriptorSet const &) = delete;

	/** Deleted copy assignment operator */
	DescriptorSet &operator=(DescriptorSet const &) = delete;

	/** Destructor */
	~DescriptorSet();

	/** Trivial accessor for the underlying descriptor set */
	auto vk() const
	{
		return allocated_descriptor_set.first;
	}

	/** Trivial accessor for the descriptor pool */
	auto get_pool() const
	{
		return allocated_descriptor_set.second;
	}

	/** Implicit boolean conversion */
	operator bool() const
	{
		return allocated_descriptor_set.first && allocated_descriptor_set.second;
	}

private:
	DescriptorAllocator *descriptor_allocator = {};

	pair<vk::DescriptorSet, vk::DescriptorPool> allocated_descriptor_set;
};

} // namespace BINDLESSVK_NAMESPACE

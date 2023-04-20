#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan's descriptor pool */
class DescriptorPool
{
public:
	/** Argumented constructor */
	DescriptorPool(Device *device, vk::DescriptorPoolCreateInfo info);

	/** Move constructor */
	DescriptorPool(DescriptorPool &&other);

	/** Move assignment operator */
	DescriptorPool &operator=(DescriptorPool &&other);

	/** Deleted copy constructor */
	DescriptorPool(DescriptorPool const &) = delete;

	/** Deleted copy assignment operator */
	DescriptorPool &operator=(DescriptorPool const &) = delete;

	/** Destructor */
	~DescriptorPool();

	/** Checks for out_of_memory and descriptor_set_count status */
	auto is_eligable_for_destruction() const -> bool;

	/** Tries to allocate a descriptor set */
	auto try_allocate_descriptor_set(vk::DescriptorSetLayout layout) -> vk::DescriptorSet;

	/** Decrements descriptor_set_count's value
	 *
	 * @note This does not releases a descriptor from pool
	 */
	void release_descriptor_set();

	/** Equality comparison operator against self */
	auto operator==(DescriptorPool const &other) const -> bool;

	/** Equality comparison operator against unwrapped descriptor pool */
	auto operator==(vk::DescriptorPool const other) const -> bool;

	/** Trivial accessorr for the underlying pool */
	auto vk() const
	{
		return descriptor_pool;
	}

private:
	Device *device = {};

	vk::DescriptorPool descriptor_pool = {};

	u32 descriptor_set_count = {};

	bool out_of_memory = {};
};

} // namespace BINDLESSVK_NAMESPACE

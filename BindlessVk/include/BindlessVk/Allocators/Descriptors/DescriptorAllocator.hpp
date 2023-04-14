#pragma once

#include "BindlessVk/Allocators/Descriptors/DescriptorPool.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Responsible for allocating and freeing descriptor sets. */
class DescriptorAllocator
{
private:
	class Pool;

public:
	/** Default constructor */
	DescriptorAllocator() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	DescriptorAllocator(VkContext const *vk_context);

	/** Move constructor */
	DescriptorAllocator(DescriptorAllocator &&other);

	/** Move assignment operator */
	DescriptorAllocator &operator=(DescriptorAllocator &&other);

	/** Deleted copy constructor */
	DescriptorAllocator(DescriptorAllocator const &) = delete;

	/** Deleted copy assignment operator */
	DescriptorAllocator &operator=(DescriptorAllocator const &other) = delete;

	/** Destructor */
	~DescriptorAllocator();

	/** Allocates a descriptor set
	 *
	 * @param descriptor_layout A descriptor set layout
	 */
	auto allocate_descriptor_set(vk::DescriptorSetLayout descriptor_layout)
	    -> pair<vk::DescriptorSet, vk::DescriptorPool>;

	/** Releases a descriptor set
	 *
	 * @param descriptor_pool The source descriptor pool of the descriptor set
	 *
	 * @note This does not free descriptor set from pool but reduces the ref count
	 * of the coresponding descriptor pool (wrapper)
	 *
	 */
	void release_descriptor_set(vk::DescriptorPool descriptorr_pool);

private:
	auto try_allocate_descriptor_set(vk::DescriptorSetAllocateInfo alloc_info)
	    -> pair<vk::DescriptorSet, vk::DescriptorPool>;

	void grab_or_create_new_pool();
	void grab_pool();

	void create_new_pools();

	void try_destroy_pool(vec<DescriptorPool>::iterator pool_it);

	auto find_pool(vk::DescriptorPool pool) -> vec<DescriptorPool>::iterator;

private:
	Device *device = {};

	vec<DescriptorPool> active_pools = {};
	vec<DescriptorPool> free_pools = {};

	vec<DescriptorPool>::iterator current_pool = {};

	u32 pool_counter = {};
};

} // namespace BINDLESSVK_NAMESPACE

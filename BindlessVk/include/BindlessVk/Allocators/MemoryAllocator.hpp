#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

class MemoryAllocator
/** Wrapper around VMA that manages memory allocations */
{
public:
	/** Default constructor */
	MemoryAllocator() = default;

	/** Argumented  constructor
	 *
	 * @param vk_context The vulkan context
	 */
	MemoryAllocator(VkContext const *vk_context);

	/** Move constructor */
	MemoryAllocator(MemoryAllocator &&other);

	/** Move assignment operator */
	MemoryAllocator &operator=(MemoryAllocator &&other);

	/** Deleted copy constructor */
	MemoryAllocator(MemoryAllocator const &) = delete;

	/** Deleted copy assignment operator */
	MemoryAllocator &operator=(MemoryAllocator const &) = delete;

	/** Destructor */
	~MemoryAllocator();

	/** Trivial accessor for the underlying allocator */
	auto vma() const
	{
		return allocator;
	}

private:
	void static allocate_memory_callback(
	    VmaAllocator VMA_NOT_NULL allocator,
	    u32 memory_type,
	    VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
	    VkDeviceSize size,
	    void *VMA_NULLABLE vma_user_data
	);

	void static free_memory_callback(
	    VmaAllocator VMA_NOT_NULL allocator,
	    u32 memory_type,
	    VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
	    VkDeviceSize size,
	    void *VMA_NULLABLE vma_user_data
	);


private:
	vma::Allocator allocator = {};
};

} // namespace BINDLESSVK_NAMESPACE

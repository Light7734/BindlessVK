#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan image and it's memory alloaction */
class Image
{
public:
	/** Default constructor */
	Image() = default;

	/** Argumented explicit constructor.
	 * meant for swapchain images because we don't allocate it's memory.
	 * Explicit so that we don't accidentaly construct a new image from image.vk()
	 *
	 * @param image A (swapchain's) vk image
	 */
	explicit Image(vk::Image image);

	/** Argumented constructor
	 *
	 * @param memory_allocator The memory allocator
	 * @param create_info Vulkan image create info
	 * @param allocate_info Vma allocation create info
	 */
	Image(
	    MemoryAllocator const *memory_allocator,
	    vk::ImageCreateInfo const &create_info,
	    vma::AllocationCreateInfo const &allocate_info
	);

	/** Move constructor */
	Image(Image &&other);

	/** Move assignment operator */
	Image &operator=(Image &&other);

	/** Deleted copy constructor */
	Image(Image const &) = delete;

	/** Deleted copy operator */
	Image &operator=(Image const &) = delete;

	/** Destructor */
	~Image();

	/** Trivial accessor for the underlying image */
	auto vk() const
	{
		return allocated_image.first;
	}

	/** Trivial accessor for allocation */
	auto get_allocation()
	{
		return allocated_image.second;
	}

	/** Checks if the image has a vma allocation
	 *
	 * @note If it doesn't, it's probably a swapchain imgae
	 */
	bool has_allocation() const
	{
		return !!allocated_image.second;
	}

private:
	MemoryAllocator const *memory_allocator = {};

	pair<vk::Image, vma::Allocation> allocated_image = {};
};

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/Texture/Image.hpp"

#include "Amender/Amender.hpp"

namespace BINDLESSVK_NAMESPACE {

Image::Image(vk::Image const image): allocated_image(image, {})
{
	ScopeProfiler _;
}

Image::Image(
    MemoryAllocator const *const memory_allocator,
    vk::ImageCreateInfo const &create_info,
    vma::AllocationCreateInfo const &allocate_info
)
    : memory_allocator(memory_allocator)
    , allocated_image(memory_allocator->vma().createImage(create_info, allocate_info))
{
	ScopeProfiler _;
}

Image::~Image()
{
	ScopeProfiler _;

	auto const &[image, allocation] = allocated_image;

	if (memory_allocator)
		memory_allocator->vma().destroyImage(image, allocation);
}


} // namespace BINDLESSVK_NAMESPACE

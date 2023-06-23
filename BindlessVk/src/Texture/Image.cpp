#include "BindlessVk/Texture/Image.hpp"



namespace BINDLESSVK_NAMESPACE {

Image::Image(vk::Image const image): allocated_image(image, {})
{
	ZoneScoped;
}

Image::Image(
    MemoryAllocator const *const memory_allocator,
    vk::ImageCreateInfo const &create_info,
    vma::AllocationCreateInfo const &allocate_info
)
    : memory_allocator(memory_allocator)
    , allocated_image(memory_allocator->vma().createImage(create_info, allocate_info))
{
	ZoneScoped;
}

Image::~Image()
{
	ZoneScoped;

	auto const &[image, allocation] = allocated_image;

	if (memory_allocator)
		memory_allocator->vma().destroyImage(image, allocation);
}


} // namespace BINDLESSVK_NAMESPACE

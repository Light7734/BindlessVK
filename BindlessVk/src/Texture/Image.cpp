#include "BindlessVk/Texture/Image.hpp"

namespace BINDLESSVK_NAMESPACE {

Image::Image(vk::Image const image): allocated_image(image, {})
{
}

Image::Image(
    MemoryAllocator const *const memory_allocator,
    vk::ImageCreateInfo const &create_info,
    vma::AllocationCreateInfo const &allocate_info
)
    : memory_allocator(memory_allocator)
    , allocated_image(memory_allocator->vma().createImage(create_info, allocate_info))
{
}

Image::Image(Image &&other)
{
	*this = std::move(other);
}

Image &Image::operator=(Image &&other)
{
	this->memory_allocator = other.memory_allocator;
	this->allocated_image = other.allocated_image;

	other.memory_allocator = {};

	return *this;
}

Image::~Image()
{
	auto const &[image, allocation] = allocated_image;

	if (memory_allocator)
		memory_allocator->vma().destroyImage(image, allocation);
}


} // namespace BINDLESSVK_NAMESPACE

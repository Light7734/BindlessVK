#pragma once

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Texture/Texture.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Loads texture files like ktx, png, etc. */
class TextureLoader
{
public:
	/** Default constructor */
	TextureLoader() = default;

	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 */
	TextureLoader(VkContext const *vk_context, MemoryAllocator const *memory_allocator);

	/** Default destructor */
	~TextureLoader() = default;

	/** Loads a texture from buffer data
	 *
	 * @param name Name of the texture
	 * @param pixels Pixel-data buffer of at least @p size bytes
	 * @param width Horizontal size of the texture
	 * @param height Vertical size of the texture
	 * @param size Size of pixel-data buffer,
	 * should be (width * height * bytes per pixel)
	 * @param type Type of the texture (eg. 2d, cubemap)
	 * @param layout Final layout of the created texture
	 */
	auto load_from_binary(
	    u8 const *pixels,
	    i32 width,
	    i32 height,
	    vk::DeviceSize size,
	    Texture::Type type,
	    Buffer *staging_buffer,
	    vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
	    str_view debug_name = default_debug_name
	) const -> Texture;

	/** Loads a texture from a ktx(khronos texture) file
	 *
	 * @param name Name of the texture
	 * @param uri Path to the .ktx file
	 * @param type Type of the texture (eg. 2d, cubemap)
	 * @param layout Final layout of the created texture
	 */
	auto load_from_ktx(
	    str_view uri,
	    Texture::Type type,
	    Buffer *staging_buffer,
	    vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal,
	    str_view debug_name = default_debug_name
	) const -> Texture;

private:
	VkContext const *vk_context = {};
	MemoryAllocator const *memory_allocator = {};
};

} // namespace BINDLESSVK_NAMESPACE

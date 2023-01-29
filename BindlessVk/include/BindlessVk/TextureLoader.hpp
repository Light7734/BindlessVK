#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Texture.hpp"
#include "BindlessVk/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

class TextureLoader
{
public:
	/** @brief Main constructor
	 * @param vk_context the vulkan context
	 */
	TextureLoader(VkContext const *vk_context);

	/** @brief Default constructor */
	TextureLoader() = default;

	/** @brief Default destructor */
	~TextureLoader() = default;

	/** @brief Loads a texture from buffer data
	 * @param name name of the texture
	 * @param pixels pixel-data buffer of at least @p size bytes
	 * @param width  horizontal size of the texture
	 * @param height vertical size of the texture
	 * @param size   size of pixel-data buffer,
	 * should be (width * height * bytes per pixel)
	 * @param type   type of the texture (eg. 2d, cubemap)
	 * @param layout final layout of the created texture
	 */
	auto load_from_binary(
	    c_str name,
	    u8 const *pixels,
	    i32 width,
	    i32 height,
	    vk::DeviceSize size,
	    Texture::Type type,
	    Buffer *staging_buffer,
	    vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal
	) const -> Texture;

	/** @brief Loads a texture from a ktx(khronos texture) file
	 * @param name   name of the texture
	 * @param uri    path to the .ktx file
	 * @param type   type of the texture (eg. 2d, cubemap)
	 * @param layout final layout of the created texture
	 */
	auto load_from_ktx(
	    c_str name,
	    c_str uri,
	    Texture::Type type,
	    Buffer *staging_buffer,
	    vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	) const -> Texture;

private:
	VkContext const *vk_context = {};
};

} // namespace BINDLESSVK_NAMESPACE

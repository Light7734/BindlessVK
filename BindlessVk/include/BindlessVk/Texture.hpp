#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Types.hpp"

namespace tinygltf {
struct Image;
}

namespace BINDLESSVK_NAMESPACE {

struct Texture
{
	enum class Type
	{
		e2D,
		e2DArray,
		eCubeMap
	};

	vk::DescriptorImageInfo descriptor_info;

	u32 width;
	u32 height;
	vk::Format format;
	u32 channels;
	u32 mip_levels;
	vk::DeviceSize size;

	vk::Sampler sampler;
	vk::ImageView image_view;
	vk::ImageLayout layout;

	AllocatedImage image;
};

class TextureSystem
{
public:
	TextureSystem() = default;

	/** Initializes the texture system
	 * @param device the bindlessvk device
	 */
	void init(Device* device);

	/** Destroys the texture system */
	void reset();

	/** Creates a texture from buffer data
	 * @param name name of the texture
	 * @param pixels pixel-data buffer of at least @p size bytes
	 * @param width  horizontal size of the texture
	 * @param height vertical size of the texture
	 * @param size   size of pixel-data buffer,
	 * should be (width * height * bytes per pixel)
	 * @param type   type of the texture (eg. 2d, cubemap)
	 * @param layout final layout of the created texture
	 */
	Texture* create_from_buffer(
	    const std::string& name,
	    u8* pixels,
	    i32 width,
	    i32 height,
	    vk::DeviceSize size,
	    Texture::Type type,
	    vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

	/** Creates a texture from a tinygltf image
	 * @param image  a tinygltf::Image image
	 * @param layout final layout of the created texture
	 *
	 * @note name of the texture will be @p image->uri
	 */
	Texture* create_from_gltf(
	    const tinygltf::Image& image,
	    vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

	/** Creates a texture from a ktx(khronos texture) file
	 * @param name   name of the texture
	 * @param uri    path to the .ktx file
	 * @param type   type of the texture (eg. 2d, cubemap)
	 * @param layout final layout of the created texture
	 */
	Texture* create_from_ktx(
	    const std::string& name,
	    const std::string& uri,
	    Texture::Type type,
	    vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

	//** @return texture named @p name */
	inline Texture* get_texture(const char* name)
	{
		return &textures[HashStr(name)];
	}

private:
	void blit_iamge(
	    vk::CommandBuffer cmd,
	    AllocatedImage image,
	    u32 mip_index,
	    i32& mip_width,
	    i32& mip_height
	);

	void transition_layout(
	    Texture& texture,
	    vk::CommandBuffer cmd_buffer,
	    u32 base_mip_level,
	    u32 level_count,
	    u32 layer_count,
	    vk::ImageLayout old_layout,
	    vk::ImageLayout new_layout
	);

	void copy_buffer_to_image(Texture& texture, vk::CommandBuffer cmdBuffer);

private:
	Device* device = {};
	std::unordered_map<u64, Texture> textures = {};
};

} // namespace BINDLESSVK_NAMESPACE
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

	uint32_t width;
	uint32_t height;
	vk::Format format;
	uint32_t channels;
	uint32_t mip_levels;
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
	  uint8_t* pixels,
	  int width,
	  int height,
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
	  struct tinygltf::Image* image,
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
		return &m_textures[HashStr(name)];
	}

private:
	void blit_iamge(
	  vk::CommandBuffer cmd,
	  AllocatedImage image,
	  uint32_t mip_index,
	  int32_t& mip_width,
	  int32_t& mip_height
	);

	void transition_layout(
	  Texture& texture,
	  vk::CommandBuffer cmd_buffer,
	  uint32_t base_mip_level,
	  uint32_t level_count,
	  uint32_t layer_count,
	  vk::ImageLayout old_layout,
	  vk::ImageLayout new_layout
	);

	void copy_buffer_to_image(Texture& texture, vk::CommandBuffer cmdBuffer);

private:
	Device* m_device                                 = {};
	std::unordered_map<uint64_t, Texture> m_textures = {};
};

} // namespace BINDLESSVK_NAMESPACE

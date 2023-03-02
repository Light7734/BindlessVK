#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Model/Model.hpp"
#include "BindlessVk/Texture/TextureLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

/**
 * @brief Loads model files like gltf, fbx, etc.
 *
 * @todo feat: support fbx
 * @todo feat: support obj
 *
 * @todo refactor: bindlessvk shouldn't be responsible for STORING textures
 */
class ModelLoader
{
public:
	/**
	 * @brief Main constructor
	 * @param vk_context the vulkan context
	 */
	ModelLoader(ref<VkContext const> vk_context);

	/** @brief Default constructor */
	ModelLoader() = default;

	/** @brief Default destructor */
	~ModelLoader() = default;

	/**
	 * @brief Loads a model from a gltf file
	 * @param name debug name attached to vulkan objects for debugging tools like renderdoc
	 * @param file_path path to the gltf model file
	 */
	auto load_from_gltf_ascii(
	    str_view file_path,
	    Buffer *staging_vertex_buffer,
	    Buffer *staging_index_buffer,
	    Buffer *staging_image_buffer,
	    str_view debug_name = default_debug_name
	) const -> Model;

	/** @todo Implement */
	auto load_from_gltf_binary() -> Model = delete;

	/** @todo Implement */
	auto load_from_fbx() -> Model = delete;

	/** @todo Implement */
	auto load_from_obj() -> Model = delete;

private:
	ref<VkContext const> vk_context = {};
	TextureLoader texture_loader = {};
};

} // namespace BINDLESSVK_NAMESPACE

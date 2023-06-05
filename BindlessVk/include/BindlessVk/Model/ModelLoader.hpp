#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Model/Model.hpp"
#include "BindlessVk/Texture/TextureLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Loads model files like gltf, fbx, etc. */
class ModelLoader
{
public:
	/** Default constructor */
	ModelLoader() = default;

	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 * @param memory_allocator Pointer to the memory allocator
	 */
	ModelLoader(VkContext const *vk_context, MemoryAllocator const *memory_allocator);

	/** Default destructor */
	~ModelLoader() = default;

	/** Loads a model from a .gltf file
	 *
	 * @param file_path null-terminated str view to path of the gltf model file
	 * @param vertex_buffer A fragmented buffer for vertex data to be written to
	 * @param index_bufer A fragmented buffer for index data to be written to
	 * @param staging_vertex_buffer A buffer to be used to stage data for uploading to gpu
	 * @param staging_index_buffer A buffer to be used to stage data for uploading to gpu
	 * @param staging_image_buffer A buffer to be used to stage data for uploading to gpu
	 * @param debug_namedebug name attached to vulkan objects for debugging tools like renderdoc
	 */
	auto load_from_gltf_ascii(
	    str_view file_path,
	    FragmentedBuffer *vertex_buffer,
	    FragmentedBuffer *index_buffer,
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
	VkContext const *vk_context = {};
	MemoryAllocator const *memory_allocator = {};
	TextureLoader texture_loader = {};
};

} // namespace BINDLESSVK_NAMESPACE

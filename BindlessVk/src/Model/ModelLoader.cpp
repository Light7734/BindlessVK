#include "BindlessVk/Model/ModelLoader.hpp"


#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Model/Loaders/GltfLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

ModelLoader::ModelLoader(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator
)
    : vk_context(vk_context)
    , texture_loader(vk_context, memory_allocator)
    , memory_allocator(memory_allocator)
{
	ZoneScoped;
}

auto ModelLoader::load_from_gltf_ascii(
    str_view const file_path,
    FragmentedBuffer *const vertex_buffer,
    FragmentedBuffer *const index_buffer,
    Buffer *const staging_vertex_buffer,
    Buffer *const staging_index_buffer,
    Buffer *const staging_image_buffer,
    str_view const debug_name /* = default_debug_name */
) const -> Model
{
	ZoneScoped;

	auto loader = GltfLoader {
		vk_context,       // curse
		memory_allocator, // you
		&texture_loader,  // clang_format!
		vertex_buffer,    // !!!!!!!!!!!!!
		index_buffer,     // ----_____----
		staging_vertex_buffer,
		staging_index_buffer,
		staging_image_buffer,

	};

	return std::move(loader.load_from_ascii(file_path, debug_name));
}

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/ModelLoader.hpp"

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/ModelLoaders/GltfLoader.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

ModelLoader::ModelLoader(
    VkContext const *const vk_context,
    TextureLoader const *const texture_loader
)
    : vk_context(vk_context)
    , texture_loader(texture_loader)
{
}

auto ModelLoader::load_from_gltf_ascii(
    c_str name,
    c_str file_path,
    Buffer *const staging_vertex_buffer,
    Buffer *const staging_index_buffer,
    Buffer *const staging_image_buffer
) const -> Model
{
	GltfLoader loader(
	    vk_context,
	    texture_loader,
	    staging_vertex_buffer,
	    staging_index_buffer,
	    staging_image_buffer
	);
	return loader.load_from_ascii(name, file_path);
}

} // namespace BINDLESSVK_NAMESPACE

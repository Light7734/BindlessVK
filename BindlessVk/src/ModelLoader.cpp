#include "BindlessVk/ModelLoader.hpp"

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/ModelLoaders/GltfLoader.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

ModelLoader::ModelLoader(Device* device, TextureLoader* texture_system)
    : device(device)
    , texture_system(texture_system)
{
}

Model ModelLoader::load_from_gltf_ascii(
    const char* name,
    const char* file_path,
    Buffer* staging_vertex_buffer,
    Buffer* staging_index_buffer
)
{
	GltfLoader loader(device, texture_system, staging_vertex_buffer, staging_index_buffer);
	return loader.load_from_ascii(name, file_path);
}

} // namespace BINDLESSVK_NAMESPACE

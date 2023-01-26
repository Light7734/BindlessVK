#include "BindlessVk/ShaderLoader.hpp"

#include "BindlessVk/ShaderLoaders/SpvLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

ShaderLoader::ShaderLoader(VkContext* vk_context): vk_context(vk_context)
{
}

Shader ShaderLoader::load_from_spv(c_str file_path)
{
	SpvLoader loader(vk_context);
	return loader.load(file_path);
}

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/ShaderLoader.hpp"

#include "BindlessVk/ShaderLoaders/SpvLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

ShaderLoader::ShaderLoader(VkContext const *const vk_context): vk_context(vk_context)
{
}

auto ShaderLoader::load_from_spv(c_str const file_path) -> Shader
{
	SpvLoader loader(vk_context);
	return loader.load(file_path);
}

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/Shader/ShaderLoader.hpp"


#include "BindlessVk/Shader/Loaders/SpvLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

ShaderLoader::ShaderLoader(VkContext const *const vk_context): vk_context(vk_context)
{
	ZoneScoped;
}

auto ShaderLoader::load_from_spv(str_view const file_path) -> Shader
{
	ZoneScoped;

	SpvLoader loader(vk_context);
	return loader.load(file_path);
}

} // namespace BINDLESSVK_NAMESPACE

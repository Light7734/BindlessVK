#include "BindlessVk/ShaderLoader.hpp"

#include "BindlessVk/ShaderLoaders/SpvLoader.hpp"

namespace BINDLESSVK_NAMESPACE {

ShaderLoader::ShaderLoader(Device* device): device(device)
{
}

Shader ShaderLoader::load_from_spv(c_str file_path)
{
	SpvLoader loader(device);
	return loader.load(file_path);
}

} // namespace BINDLESSVK_NAMESPACE

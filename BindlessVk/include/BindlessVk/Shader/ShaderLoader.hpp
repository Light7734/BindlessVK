#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Shader/Shader.hpp"

namespace BINDLESSVK_NAMESPACE {

class ShaderLoader
{
public:
	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 */
	ShaderLoader(VkContext const *vk_context);

	/** Default constructor */
	ShaderLoader() = default;

	/** Default destructor */
	~ShaderLoader() = default;

	/** Loads a shader from spv file
	 *
	 * @param file_path Path to the spv shader file
	 */
	auto load_from_spv(str_view file_path) -> Shader;

	auto load_from_glsl(str_view file_path) -> Shader = delete;

	auto load_from_hlsl(str_view file_path) -> Shader = delete;

private:
	VkContext const *vk_context = {};
};

} // namespace BINDLESSVK_NAMESPACE

#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Shader/Shader.hpp"

namespace BINDLESSVK_NAMESPACE {

class ShaderLoader
{
public:
	/**
	 * @brief Main constructor
	 * @param vk_context the vulkan context
	 */
	ShaderLoader(ref<VkContext const> vk_context);

	/** @brief Default constructor */
	ShaderLoader() = default;

	/** @brief Default destructor */
	~ShaderLoader() = default;

	/**
	 * @brief Loads a shader from spv file
	 * @param file_path path to the spv shader file
	 */
	auto load_from_spv(str_view file_path) -> Shader;

	/** @todo Implement */
	auto load_from_glsl(str_view file_path) -> Shader = delete;

	/** @todo Implement  */
	auto load_from_hlsl(str_view file_path) -> Shader = delete;

private:
	ref<VkContext const> vk_context = {};
};

} // namespace BINDLESSVK_NAMESPACE

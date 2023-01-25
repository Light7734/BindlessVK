#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Shader.hpp"

namespace BINDLESSVK_NAMESPACE {

class ShaderLoader
{
public:
	/**
	 * @brief Main constructor
	 * @param device the bindlessvk device
	 */
	ShaderLoader(Device* device);

	/** @brief Default constructor */
	ShaderLoader() = default;

	/** @brief Default destructor */
	~ShaderLoader() = default;

	/**
	 * @brief Loads a shader from spv file
	 * @param file_path path to the spv shader file
	 */
	Shader load_from_spv(c_str file_path);

	/** @todo Implement */
	Shader load_from_glsl(c_str file_path) = delete;

	/** @todo Implement  */
	Shader load_from_hlsl(c_str file_path) = delete;

private:
	Device* device = {};
};

} // namespace BINDLESSVK_NAMESPACE

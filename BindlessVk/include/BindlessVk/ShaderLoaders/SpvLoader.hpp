#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Shader.hpp"

#include <spirv_reflect.h>

static_assert(
    SPV_REFLECT_RESULT_SUCCESS == false,
    "SPV_REFLECT_RESULT_SUCCESS was suppoed to be 0 (false), but it isn't"
);

namespace BINDLESSVK_NAMESPACE {

class SpvLoader
{
public:
	SpvLoader(Device* device);

	Shader load(c_str path);

private:
	void load_code(c_str path);
	void reflect_code();
	void create_vulkan_shader_module();

	void reflect_descriptor_sets();
	void reflect_shader_stage();

	vec<vk::DescriptorSetLayoutBinding> reflect_descriptor_set_bindings(
	    SpvReflectDescriptorSet* spv_set
	);


	vk::DescriptorSetLayoutBinding extract_descriptor_set_binding(
	    SpvReflectDescriptorBinding* binding
	);

private:
	Device* device = {};
	Shader shader = {};
	vec<u32> code = {};
	SpvReflectShaderModule reflection = {};
};

} // namespace BINDLESSVK_NAMESPACE

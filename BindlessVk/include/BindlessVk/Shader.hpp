#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Shader
{
	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	arr<vec<vk::DescriptorSetLayoutBinding>, 2u> descriptor_sets_bindings;
};

} // namespace BINDLESSVK_NAMESPACE

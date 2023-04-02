#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

class Instance
{
public:
	struct Requirements
	{
		vec<c_str> extensions;
		vec<c_str> layers;
	};

public:
	Instance() = default;
	Instance(Requirements const &requirements);

	Instance(Instance &&other);
	Instance &operator=(Instance &&other);

	Instance(Instance const &) = delete;
	Instance &operator=(Instance const &) = delete;

	~Instance();

	auto vk() const
	{
		return instance;
	}

	auto get_layers()
	{
		return layers;
	}

	auto get_extensions()
	{
		return extensions;
	}

	auto get_proc_addr(str_view proc_name) const
	{
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(instance, proc_name.data());
	}

	operator vk::Instance()
	{
		return instance;
	}

	operator VkInstance()
	{
		return static_cast<VkInstance>(instance);
	}

private:
	void load_functions();

	void check_layer_support();

	void create_instance();

	auto has_layer(c_str layer) const -> bool;

private:
	vk::Instance instance = {};
	vec<c_str> extensions = {};
	vec<c_str> layers = {};
};

} // namespace BINDLESSVK_NAMESPACE

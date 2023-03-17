#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

class Instance
{
public:
	Instance(vec<c_str> const &extensions, vec<c_str> const &layers);

	~Instance();

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

	auto enumerate_physical_devices()
	{
		return instance.enumeratePhysicalDevices();
	}

	auto create_debug_messenger(vk::DebugUtilsMessengerCreateInfoEXT create_info) const
	{
		return instance.createDebugUtilsMessengerEXT(create_info);
	}

	void destroy_debug_messenger(vk::DebugUtilsMessengerEXT messenger) const
	{
		instance.destroyDebugUtilsMessengerEXT(messenger);
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

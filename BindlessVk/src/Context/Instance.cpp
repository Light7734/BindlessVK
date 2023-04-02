#include "BindlessVk/Context/Instance.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {

Instance::Instance(Requirements const &requirements)
    : extensions(requirements.extensions)
    , layers(requirements.layers)
{
	load_functions();

	check_layer_support();

	create_instance();
}

Instance::Instance(Instance &&other)
{
	*this = std::move(other);
}

Instance &Instance::operator=(Instance &&other)
{
	this->instance = other.instance;
	this->extensions = std::move(other.extensions);
	this->layers = std::move(other.layers);

	other.instance = vk::Instance {};
	other.extensions = {};
	other.layers = {};

	return *this;
}

Instance::~Instance()
{
	if (!instance)
		return;

	instance.destroy();
}

void Instance::load_functions()
{
	VULKAN_HPP_DEFAULT_DISPATCHER.init(
	    vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")
	);
}

void Instance::check_layer_support()
{
	for (auto const layer : layers)
		if (!has_layer(layer))
			assert_fail("Required layer: {} is not supported", layer);
}

void Instance::create_instance()
{
	auto const application_info = vk::ApplicationInfo {
		"BindlessVk",
		VK_MAKE_VERSION(1, 0, 0),
		"BindlessVk", //
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_3,
	};

	auto const instance_info = vk::InstanceCreateInfo {
		{},
		&application_info,
		layers,
		extensions,
	};

	assert_false(vk::createInstance(&instance_info, nullptr, &instance));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

auto Instance::has_layer(c_str layer) const -> bool
{
	for (auto const &layer_properties : vk::enumerateInstanceLayerProperties())
		if (strcmp(layer, layer_properties.layerName))
			return true;

	return false;
}

} // namespace BINDLESSVK_NAMESPACE

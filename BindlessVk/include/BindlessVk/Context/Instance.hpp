#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan instance */
class Instance
{
public:
	/** Requirements that needs to be met for vulkan instance to be considered usable */
	struct Requirements
	{
		vec<c_str> extensions;
		vec<c_str> layers;
	};

	/** Severity and type filter to determine the verbosity of debug callback */
	struct Filter
	{
		vk::DebugUtilsMessageSeverityFlagsEXT severity_flags;
		vk::DebugUtilsMessageTypeFlagsEXT type_flags;
	};

public:
	/** Default constructor */
	Instance() = default;

	/** Argumented constructor
	 *
	 * @param requirements The required layers and extensions
	 * @param filter Validation layer callback filter
	 */
	Instance(Requirements const &requirements, Filter const &filter);

	/** Move constructor */
	Instance(Instance &&other);

	/** Move assignment operator */
	Instance &operator=(Instance &&other);

	/** Deleted copy constructor*/
	Instance(Instance const &) = delete;

	/** Deleted copy assignment operator */
	Instance &operator=(Instance const &) = delete;

	/** Destructor */
	~Instance();

	/** Trivial accessor for the underlying instance */
	auto vk() const
	{
		return instance;
	}

	/** Trivial accessor for requirements */
	auto get_required_layers()
	{
		return requirements;
	}

	/** Returns instance proc address of @a proc_name
	 *
	 * @param proc_name A null-terminated str view
	 */
	auto get_proc_addr(str_view proc_name) const
	{
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(instance, proc_name.data());
	}

private:
	void load_functions();

	void check_layer_support();

	void create_instance();

	auto has_layer(c_str layer) const -> bool;

private:
	vk::Instance instance = {};
	Requirements requirements = {};
	vk::DebugUtilsMessengerEXT messenger = {};
};

} // namespace BINDLESSVK_NAMESPACE

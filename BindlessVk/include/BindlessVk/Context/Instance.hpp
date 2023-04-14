#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan instance */
class Instance
{
public:
	struct Requirements
	{
		vec<c_str> extensions;
		vec<c_str> layers;
	};

public:
	/** Default constructor */
	Instance() = default;

	/** Argumented constructor
	 *
	 * @param requirements The required layers and extensions
	 */
	Instance(Requirements const &requirements);

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
};

} // namespace BINDLESSVK_NAMESPACE

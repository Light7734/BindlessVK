#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Instance.hpp"

#include <any>

namespace BINDLESSVK_NAMESPACE {

enum class DebugCallbackSource
{
	eBindlessVk,
	eValidationLayers,
	eVma,

	nCount
};

/** Responsible for logging and handling vulkan debug utils extension */
class DebugUtils
{
public:
	/** Data required for logging */
	struct Callback
	{
		fn<void(DebugCallbackSource, LogLvl, str const &, std::any)> function;
		std::any user_data;
	};

	/** Severity and type filter to determine the verbosity of debug callback */
	struct Filter
	{
		vk::DebugUtilsMessageSeverityFlagsEXT severity_flags;
		vk::DebugUtilsMessageTypeFlagsEXT type_flags;
	};

public:
	/** Default constructor */
	DebugUtils() = default;

	/** Argumented constructor
	 *
	 * @param instance Pointer to the instance
	 * @param callback Message callback function and user data
	 * @param filter Message callback filter
	 */
	DebugUtils(Instance *instance, Callback const &callback, Filter const &filter);

	/** Default move constructor */
	DebugUtils(DebugUtils &&other) = default;

	/** Default move assignment operator */
	DebugUtils &operator=(DebugUtils &&other) = default;

	/** Deleted copy constructor */
	DebugUtils(DebugUtils const &) = delete;

	/** Deleted copy assignment operator */
	DebugUtils &operator=(DebugUtils const &) = delete;

	/** Destructor */
	~DebugUtils();

	/** Invokes the message callback with log data
	 *
	 * @param lvl Severity of the message
	 * @param fmt Format string passed to fmt::format
	 * @param args Variadic arguments forwarded to fmt::format
	 *
	 * @warning Internal
	 */
	template<typename... Args>
	void log(LogLvl lvl, fmt::format_string<Args...> fmt, Args &&...args) const
	{
		callback->function(
		    DebugCallbackSource::eBindlessVk,
		    lvl,
		    fmt::format(fmt, std::forward<Args>(args)...),
		    callback->user_data
		);
	}

	/** Sets the vulkan object's name
	 *
	 * @param device The vulkan device
	 * @param object A vulkan hpp object
	 * @param name A null terminated str view to name of the object
	 */
	template<typename T>
	void set_object_name(vk::Device device, T object, str_view name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.objectType,
		    (u64)((typename T::NativeType)(object)),
		    name.data(),
		});
	}

	/** Trivial address-accessor for callback */
	auto get_callback()
	{
		return callback.get();
	}

private:
	auto static validation_layers_callback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	    VkDebugUtilsMessageTypeFlagsEXT message_types,
	    VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
	    void *vulkan_user_data
	) -> VkBool32;


	auto static parse_message_type(VkDebugUtilsMessageTypeFlagsEXT message_types) -> c_str;
	auto static parse_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity)
	    -> LogLvl;

private:
	tidy_ptr<Instance> instance = {};
	vk::DebugUtilsMessengerEXT messenger = {};

	// Needs to live on the heap to preserve address after move
	scope<Callback> callback = {};
};

} // namespace BINDLESSVK_NAMESPACE

#include "BindlessVk/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

#ifdef BVK_NOT_CONFIGURED
std::shared_ptr<spdlog::logger> Logger::s_Logger = {};
#endif

} // namespace BINDLESSVK_NAMESPACE

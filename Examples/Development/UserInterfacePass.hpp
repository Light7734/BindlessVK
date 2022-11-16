#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Scene/Scene.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

inline void UserInterfacePassUpdate(const bvk::RenderContext& context)
{
	ImGui::Render();
}

inline void UserInterfacePassRender(const bvk::RenderContext& context)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.cmd);
}

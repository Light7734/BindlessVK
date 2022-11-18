#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Scene/Scene.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

inline void UserInterfacePassBeginFrame(const bvk::RenderContext& context)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

inline void UserInterfacePassUpdate(const bvk::RenderContext& context)
{
	ImGui::Render();
}

inline void UserInterfacePassRender(const bvk::RenderContext& context)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.cmd);
}

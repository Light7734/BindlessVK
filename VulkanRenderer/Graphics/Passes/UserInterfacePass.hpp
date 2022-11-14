#pragma once

#include "Core/Base.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Types.hpp"
#include "Scene/Scene.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

inline void UserInterfacePassUpdate(const RenderContext& context)
{
	ImGui::Render();
}

inline void UserInterfacePassRender(const RenderContext& context)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.cmd);
}

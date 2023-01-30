#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "Framework/Scene/Scene.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

inline void user_interface_pass_update(
  bvk::VkContext *vk_context,
  bvk::RenderGraph *render_graph,
  bvk::Renderpass *render_pass,
  u32 frame_index,
  void *user_pointer
)
{
	ImGui::Render();
}

inline void user_interface_pass_render(
  bvk::VkContext *vk_context,
  bvk::RenderGraph *render_graph,
  bvk::Renderpass *render_pass,
  vk::CommandBuffer cmd,
  u32 frame_index,
  u32 image_index,
  void *user_pointer
)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

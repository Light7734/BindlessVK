#pragma once

#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Scene/Scene.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

inline void user_interface_pass_begin_frame(
  bvk::Device* device,
  bvk::RenderGraph* render_graph,
  bvk::Renderpass* render_pass,
  uint32_t frame_index,
  void* user_pointer
)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

inline void user_interface_pass_update(
  bvk::Device* device,
  bvk::RenderGraph* render_graph,
  bvk::Renderpass* render_pass,
  uint32_t frame_index,
  void* user_pointer
)
{
	ImGui::Render();
}

inline void user_interface_pass_render(
  bvk::Device* device,
  bvk::RenderGraph* render_graph,
  bvk::Renderpass* render_pass,
  vk::CommandBuffer cmd,
  uint32_t frame_index,
  uint32_t image_index,
  void* user_pointer
)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

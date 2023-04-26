#include "Rendergraphs/Passes/UserInterface.hpp"

#include "Framework/UserInterface/ImguiGlfwBackend.hpp"
#include "Framework/UserInterface/ImguiVulkanBackend.hpp"

#include <imgui.h>


UserInterfacePass::UserInterfacePass(bvk::VkContext const *const vk_context)
    : bvk::Renderpass(vk_context)
{
}

void UserInterfacePass::on_setup()
{
}

void UserInterfacePass::on_frame_prepare(u32 const frame_index, u32 const image_index)
{
	ImGui::Render();
}

void UserInterfacePass::on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
}

void UserInterfacePass::on_frame_graphics(
    vk::CommandBuffer const cmd,
    u32 const frame_index,
    u32 const image_index
)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

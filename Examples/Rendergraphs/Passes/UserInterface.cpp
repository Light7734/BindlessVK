#include "Rendergraphs/Passes/UserInterface.hpp"

#include "Framework/UserInterface/ImguiGlfwBackend.hpp"
#include "Framework/UserInterface/ImguiVulkanBackend.hpp"

#include <imgui.h>

UserInterfacePass::UserInterfacePass(bvk::VkContext const *const vk_context)
    : bvk::RenderNode(vk_context)
{
}

UserInterfacePass::UserInterfacePass(UserInterfacePass &&other)
{
	*this = std::move(other);
}

UserInterfacePass &UserInterfacePass::operator=(UserInterfacePass &&other)
{
	bvk::RenderNode::operator=(std::move(other));
	return *this;
}

void UserInterfacePass::on_setup(bvk::RenderNode *const parent)
{
}

void UserInterfacePass::on_frame_prepare(u32 const frame_index, u32 const image_index)
{
}

void UserInterfacePass::on_frame_compute(
    vk::CommandBuffer const cmd,
    u32 frame_index,
    u32 const image_index
)
{
}

void UserInterfacePass::on_frame_graphics(
    vk::CommandBuffer const cmd,
    u32 const frame_index,
    u32 const image_index
)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

#include "Framework/Utils/CVar.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

auto CVar::get_instance() -> CVar *
{
	auto static instance = CVar {};
	return &instance;
}

void CVar::create_impl(
    CVarType type,
    str_view const name,
    str_view const description,
    CVarVal default_value,
    CVarVal current_value
)
{
	vars[hash_str(name)] = CVarEntry {
		name, description, type, current_value, default_value,
	};
}

void CVar::set_impl(str_view const name, CVarVal value)
{
	vars[hash_str(name)].current_value = value;
}

void CVar::reset_impl(str_view const name)
{
	vars[hash_str(name)].current_value = vars[hash_str(name)].default_value;
}

auto CVar::get_impl(str_view const name) -> CVarVal
{
	return vars[hash_str(name)].current_value;
}

void CVar::draw_imgui_editor_impl()
{
	ImGui::Begin("Console Variables");
	ImGui::Text("%lu", vars.size());

	for (auto &[key, var] : vars)
	{
		switch (var.type)
		{
		case CVarType::Int:
			ImGui::DragInt(var.name.c_str(), static_cast<i32 *>(var.current_value));
			break;

		case CVarType::Float:
			ImGui::DragFloat(var.name.c_str(), static_cast<f32 *>(var.current_value));
			break;

		case CVarType::String:
			ImGui::InputText(var.name.c_str(), static_cast<str *>(var.current_value));
			break;

		case CVarType::Boolean:
			ImGui::Checkbox(var.name.c_str(), static_cast<bool *>(var.current_value));
			break;

		default: assert_fail("Unknown CVarEntry value type");
		}
	}

	ImGui::End();
}

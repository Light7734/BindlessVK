#include "Framework/Utils/CVar.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

CVar* CVar::get_instance()
{
	static CVar instance;
	return &instance;
}

void CVar::create_impl(
  CVarType type,
  const char* name,
  const char* description,
  CVarVal default_value,
  CVarVal current_value
)
{
	m_vars[name] = { type, current_value, default_value };
}

void CVar::set_impl(const char* name, CVarVal value)
{
	m_vars[name].current_value = value;
}

void CVar::reset_impl(const char* name)
{
	m_vars[name].current_value = m_vars[name].default_value;
}

CVarVal CVar::get_impl(const char* name)
{
	return m_vars[name].current_value;
}

void CVar::draw_imgui_editor_impl()
{
	ImGui::Begin("Console Variables");
	ImGui::Text("%lu", m_vars.size());

	for (auto& it : m_vars)
	{
		switch (it.second.type)
		{
		case CVarType::Int:
			ImGui::DragInt(it.first.c_str(), static_cast<int*>(it.second.current_value));
			break;

		case CVarType::Float:
			ImGui::DragFloat(
			  it.first.c_str(),
			  static_cast<float*>(it.second.current_value)
			);

			break;
		case CVarType::String:
			ImGui::InputText(
			  it.first.c_str(),
			  static_cast<std::string*>(it.second.current_value)
			);
			break;

		case CVarType::Boolean:
			ImGui::Checkbox(
			  it.first.c_str(),
			  static_cast<bool*>(it.second.current_value)
			);
			break;

		default: ASSERT(false, "Unknown CVarEntry value type");
		}
	}
	ImGui::End();
}

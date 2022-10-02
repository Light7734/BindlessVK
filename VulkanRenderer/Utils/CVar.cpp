#include "Utils/CVar.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

CVar* CVar::GetInstance()
{
	static CVar instance;
	return &instance;
}

void CVar::CreateImpl(CVarType type, const char* name, const char* description, CVarVal defaultValue, CVarVal currentValue)
{
	m_Vars[name] = { type, currentValue, defaultValue };
}

void CVar::SetImpl(const char* name, CVarVal value)
{
	m_Vars[name].currentValue = value;
}

void CVar::ResetImpl(const char* name)
{
	m_Vars[name].currentValue = m_Vars[name].defaultValue;
}

CVarVal CVar::GetImpl(const char* name)
{
	return m_Vars[name].currentValue;
}

void CVar::DrawImguiEditorImpl()
{
	ImGui::Begin("Console Variables");
	ImGui::Text("%lu", m_Vars.size());

	for (auto& it : m_Vars)
	{
		switch (it.second.type)
		{
		case CVarType::Int:
			ImGui::DragInt(it.first.c_str(), static_cast<int*>(it.second.currentValue));
			break;

		case CVarType::Float:
			ImGui::DragFloat(it.first.c_str(), static_cast<float*>(it.second.currentValue));

			break;
		case CVarType::String:
			ImGui::InputText(it.first.c_str(), static_cast<std::string*>(it.second.currentValue));
			break;

		case CVarType::Boolean:
			ImGui::Checkbox(it.first.c_str(), static_cast<bool*>(it.second.currentValue));
			break;

		default:
			ASSERT(false, "Unknown CVarEntry value type");
		}
	}
	ImGui::End();
}

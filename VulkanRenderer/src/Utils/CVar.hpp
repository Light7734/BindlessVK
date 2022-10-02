#pragma once
#include "Core/Base.hpp"

#include <iostream>
#include <map>
#include <variant>

enum class CVarType : uint8_t
{
	Int,
	Float,
	String,
	Boolean,
};


class CVarVal
{
public:
	CVarVal(float value)
	{
		m_Value = value;
	}

	CVarVal(int value)
	{
		m_Value = value;
	}

	CVarVal(std::string value)
	{
		m_Value = value;
	}

	inline operator bool() const { return std::get<bool>(m_Value); }
	inline operator bool*() { return std::get_if<bool>(&m_Value); }

	inline operator float() const { return std::get<float>(m_Value); }
	inline operator float*() { return std::get_if<float>(&m_Value); }

	inline operator int() const { return std::get<int>(m_Value); }
	inline operator int*() { return std::get_if<int>(&m_Value); }

	inline operator std::string() const { return std::get<std::string>(m_Value); }
	inline operator std::string*() { return std::get_if<std::string>(&m_Value); }

private:
	std::variant<bool, int, float, std::string> m_Value;
};

class CVar
{
public:
	static inline void Create(CVarType type, const char* name, const char* description, CVarVal defaultValue, CVarVal currentValue)
	{
		GetInstance()->CreateImpl(type, name, description, defaultValue, currentValue);
	}

	static inline void Set(const char* name, CVarVal value)
	{
		GetInstance()->SetImpl(name, value);
	}


	static inline void Reset(const char* name)
	{
		GetInstance()->ResetImpl(name);
	}

	static inline CVarVal Get(const char* name)
	{
		return GetInstance()->GetImpl(name);
	}

	static inline void DrawImguiEditor()
	{
		GetInstance()->DrawImguiEditorImpl();
	}

private:
	void CreateImpl(CVarType type, const char* name, const char* description, CVarVal defaultValue, CVarVal currentValue);

	void SetImpl(const char* name, CVarVal value);
	void ResetImpl(const char* name);

	CVarVal GetImpl(const char* name);

	void DrawImguiEditorImpl();

	static CVar* GetInstance();

private:
	struct CVarEntry
	{
		CVarEntry()
		    : type(CVarType::Boolean)
		    , currentValue(false)
		    , defaultValue(false)
		{
		}

		CVarEntry(CVarType type, CVarVal currentValue, CVarVal defaultValue)
		    : type(type)
		    , currentValue(currentValue)
		    , defaultValue(defaultValue)
		{
		}

		CVarType type;
		CVarVal currentValue;
		CVarVal defaultValue;
	};

	std::map<std::string, CVarEntry> m_Vars;

	static CVar m_Instance;
};

class AutoCVar
{
public:
	AutoCVar(CVarType type, const char* name, const char* description, CVarVal defaultValue, CVarVal currentValue)
	{
		std::cout << "Should this work????" << std::endl;
		CVar::Create(type, name, description, defaultValue, currentValue);
	}
};

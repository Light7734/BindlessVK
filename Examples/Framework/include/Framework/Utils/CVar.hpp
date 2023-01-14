#pragma once

#include "Framework/Common/Common.hpp"

#include <map>
#include <variant>

enum class CVarType : uint8_t
{
	Boolean,
	Float,
	Int,
	String,
};

class CVarVal
{
public:
	CVarVal(bool value)
	{
		m_value = value;
	}

	CVarVal(float value)
	{
		m_value = value;
	}

	CVarVal(int value)
	{
		m_value = value;
	}

	CVarVal(std::string value)
	{
		m_value = value;
	}

	inline operator bool() const
	{
		return std::get<bool>(m_value);
	}

	inline operator bool*()
	{
		return std::get_if<bool>(&m_value);
	}

	inline operator float() const
	{
		return std::get<float>(m_value);
	}

	inline operator float*()
	{
		return std::get_if<float>(&m_value);
	}

	inline operator int() const
	{
		return std::get<int>(m_value);
	}

	inline operator int*()
	{
		return std::get_if<int>(&m_value);
	}

	inline operator std::string() const
	{
		return std::get<std::string>(m_value);
	}

	inline operator std::string*()
	{
		return std::get_if<std::string>(&m_value);
	}

private:
	std::variant<bool, int, float, std::string> m_value;
};

class CVar
{
public:
	static inline void create(
	  CVarType type,
	  const char* name,
	  const char* description,
	  CVarVal default_value,
	  CVarVal current_value
	)
	{
		get_instance()->create_impl( // we're no strangers to love
		  type,
		  name,
		  description,
		  default_value,
		  current_value
		);
	}

	static inline void set(const char* name, CVarVal value)
	{
		get_instance()->set_impl(name, value);
	}

	static inline void reset(const char* name)
	{
		get_instance()->reset_impl(name);
	}

	static inline CVarVal get(const char* name)
	{
		return get_instance()->get_impl(name);
	}

	static inline void draw_imgui_editor()
	{
		get_instance()->draw_imgui_editor_impl();
	}

private:
	static CVar* get_instance();

	void create_impl(
	  CVarType type,
	  const char* name,
	  const char* description,
	  CVarVal default_value,
	  CVarVal current_value
	);

	void set_impl(const char* name, CVarVal value);

	void reset_impl(const char* name);

	CVarVal get_impl(const char* name);

	void draw_imgui_editor_impl();

private:
	struct CVarEntry
	{
		CVarEntry(): type(CVarType::Boolean), current_value(false), default_value(false)
		{
		}

		CVarEntry(CVarType type, CVarVal current_value, CVarVal default_value)
		  : type(type)
		  , current_value(current_value)
		  , default_value(default_value)
		{
		}

		CVarType type;
		CVarVal current_value;
		CVarVal default_value;
	};

	std::map<std::string, CVarEntry> vars;

	static CVar instance;
};

class AutoCVar
{
public:
	AutoCVar(
	  CVarType type,
	  const char* name,
	  const char* description,
	  CVarVal default_value,
	  CVarVal current_value
	)
	{
		CVar::create(type, name, description, default_value, current_value);
	}
};

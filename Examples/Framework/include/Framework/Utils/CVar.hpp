#pragma once

#include "Framework/Common/Common.hpp"

#include <map>
#include <variant>

enum class CVarType : u8
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
		value = value;
	}

	CVarVal(f32 value)
	{
		value = value;
	}

	CVarVal(i32 value)
	{
		value = value;
	}

	CVarVal(str value)
	{
		value = value;
	}

	inline operator bool() const
	{
		return std::get<bool>(value);
	}

	inline operator bool *()
	{
		return std::get_if<bool>(&value);
	}

	inline operator f32() const
	{
		return std::get<f32>(value);
	}

	inline operator f32 *()
	{
		return std::get_if<f32>(&value);
	}

	inline operator i32() const
	{
		return std::get<i32>(value);
	}

	inline operator i32 *()
	{
		return std::get_if<i32>(&value);
	}

	inline operator str() const
	{
		return std::get<str>(value);
	}

	inline operator str *()
	{
		return std::get_if<str>(&value);
	}

private:
	std::variant<bool, i32, f32, str> value;
};

class CVar
{
public:
	static inline void create(
	    CVarType type,
	    str_view name,
	    str_view description,
	    CVarVal default_value,
	    CVarVal current_value
	)
	{
		get_instance()->create_impl(type, name, description, default_value, current_value);
	}

	static inline void set(str_view name, CVarVal value)
	{
		get_instance()->set_impl(name, value);
	}

	static inline void reset(str_view name)
	{
		get_instance()->reset_impl(name);
	}

	static inline CVarVal get(str_view name)
	{
		return get_instance()->get_impl(name);
	}

	static inline void draw_imgui_editor()
	{
		get_instance()->draw_imgui_editor_impl();
	}

private:
	static CVar *get_instance();

	void create_impl(
	    CVarType type,
	    str_view name,
	    str_view description,
	    CVarVal default_value,
	    CVarVal current_value
	);

	void set_impl(str_view name, CVarVal value);

	void reset_impl(str_view name);

	CVarVal get_impl(str_view name);

	void draw_imgui_editor_impl();

private:
	struct CVarEntry
	{
		CVarEntry(): type(CVarType::Boolean), current_value(false), default_value(false)
		{
		}

		CVarEntry(
		    str_view name,
		    str_view description,
		    CVarType type,
		    CVarVal current_value,
		    CVarVal default_value
		)
		    : name(name)
		    , description(description)
		    , type(type)
		    , current_value(current_value)
		    , default_value(default_value)
		{
		}

		str name;
		str description;

		CVarType type;
		CVarVal current_value;
		CVarVal default_value;
	};

	hash_map<u64, CVarEntry> vars;
};

class AutoCVar
{
public:
	AutoCVar(
	    CVarType type,
	    str_view name,
	    str_view description,
	    CVarVal default_value,
	    CVarVal current_value
	)
	{
		CVar::create(type, name, description, default_value, current_value);
	}
};

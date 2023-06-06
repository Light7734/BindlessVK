#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <source_location>

// primitve type aliases
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int8_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using usize = size_t;

// c-style string alias
using c_str = char const *;

// standard type aliases
using str = std::string;
using str_view = std::string_view;

using ::std::chrono::microseconds;
using ::std::chrono::steady_clock;
using ::std::chrono::time_point_cast;

struct ScopeResult
{
	c_str name;

	u64 start;
	u64 duration;
	u64 thread_id;
};

class Profiler
{
public:
	friend class ScopeProfiler;

public:
	void static initialize(std::filesystem::path path)
	{
		instance = new Profiler(path);
	}

	void static terminate()
	{
		delete instance;
	}

	void static submit_scope_result(ScopeResult const &result)
	{
		instance->ofstream << ",{";
		instance->ofstream << "\"name\":\"" << result.name << "\",";
		instance->ofstream << "\"cat\": \"scope\",";
		instance->ofstream << "\"ph\": \"X\",";
		instance->ofstream << "\"ts\":" << result.start << ",";
		instance->ofstream << "\"dur\":" << result.duration << ",";
		instance->ofstream << "\"pid\":0,";
		instance->ofstream << "\"tid\":" << result.thread_id << "";
		instance->ofstream << "}";
	}

private:
	Profiler(std::filesystem::path path)
	{
		Profiler::instance = this;
		create_file(path);
		initiate_profile();
	}

	~Profiler()
	{
		ofstream << "]}";
		ofstream.close();

		Profiler::instance = {};
	}

private:
	void create_file(std::filesystem::path path)
	{
		ofstream.open(path, std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);

		int *a = nullptr;

		if (!ofstream.is_open())
			*a = 0; // seg;
	}

	void initiate_profile()
	{
		auto const since_epoch = time_point_cast<microseconds>(steady_clock::now())
		                             .time_since_epoch()
		                             .count();

		ofstream << "{\"traceEvents\":[";

		ofstream << "{";
		ofstream << "\"name\":\"init\",";
		ofstream << "\"cat\": \"scope\",";
		ofstream << "\"ph\": \"X\",";
		ofstream << "\"ts\":" << since_epoch << ",";
		ofstream << "\"dur\":" << 0ull << ",";
		ofstream << "\"pid\":0,";
		ofstream << "\"tid\":" << 0ull << "";
		ofstream << "}";
	}


private:
	Profiler static *instance;

	std::ofstream ofstream = {};
};

class ScopeProfiler
{
public:
	ScopeProfiler(c_str function_name = std::source_location::current().function_name())
	    : name(function_name)
	    , start(since_epoch())
	{
	}

	~ScopeProfiler()
	{
		auto const duration = since_epoch() - start;

		auto const result = ScopeResult {
			name,
			start,
			duration,
			{},
		};

		Profiler::submit_scope_result(result);
	}

private:
	u64 since_epoch()
	{
		return time_point_cast<microseconds>(steady_clock::now()).time_since_epoch().count();
	}

private:
	c_str name;
	u64 start;
};

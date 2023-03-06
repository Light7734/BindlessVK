#pragma once

#include "Framework/Common/Common.hpp"

#include <chrono>

// Simple timer class to keep track of the elapsed time
class Timer
{
public:
	Timer(): start(std::chrono::steady_clock::now())
	{
	}

	// Re-captures starting time point to steady_clock::now
	inline void reset()
	{
		start = std::chrono::steady_clock::now();
	}

	// Returns the elapsed time in seconds
	inline float elapsed_time()
	{
		auto const end = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0f;
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> start = {};
};

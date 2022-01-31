#pragma once

#include "Core/Base.hpp"

#include <chrono>

// Simple timer class to keep track of the elapsed time
class Timer
{
public:
	Timer()
	{
		Reset();
	}

	// Re-captures starting time point to steady_clock::now
	void Reset()
	{
		m_Start = std::chrono::steady_clock::now();
	}

	// Returns the elapsed time in seconds
	float ElapsedTime()
	{
		const auto End = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(End - m_Start).count() / 1000.0f;
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> m_Start;
};

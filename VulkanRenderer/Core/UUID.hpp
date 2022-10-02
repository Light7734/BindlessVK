#pragma once

#include "Core/Base.hpp"

#include <random>

class UUID
{
public:
	UUID(uint64_t uuid = -1);

	operator uint64_t() const { return m_UUID; }

private:
	static std::mt19937_64 s_Engine;
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	uint64_t m_UUID = {};
};

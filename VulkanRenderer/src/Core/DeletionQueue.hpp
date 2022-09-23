#pragma once

#include "Core/Base.hpp"

#include <functional>
#include <vector>

/** @brief Simple deletion queue for vulkan objects
* Use for coupling creation/allocation of vulkan objects with their deletion/freetion in one place
* So that we don't need to remember to delete it in another place(usually the destructor)
* @warn don't use this on performance critical code */
class DeletionQueue
{
public:
	DeletionQueue(const char* debugName)
	    : m_DebugName(debugName)
	{
	}

	DeletionQueue(const DeletionQueue&)            = delete;
	DeletionQueue& operator=(const DeletionQueue&) = delete;

	~DeletionQueue()
	{
		if (m_Deletors.empty())
		{
			LOG(warn, "Deletion queue {} destructed without flushing {} deletors", m_DebugName, m_Deletors.size());
		}
	}

	inline void Enqueue(std::function<void()> deletor)
	{
		m_Deletors.push_back(deletor);
	}

	void Flush()
	{
		LOG(trace, "Flushing deletion queue {} with {} deletors", m_DebugName, m_Deletors.size());

		for (size_t i = m_Deletors.size(); i-- > 0ull;)
		{
			m_Deletors[i]();
		}
	}

private:
	const char* m_DebugName;
	std::vector<std::function<void()>> m_Deletors = {};
};

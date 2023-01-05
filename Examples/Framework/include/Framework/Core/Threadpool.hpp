#pragma once

#include "Framework/Core/Common.hpp"

#include <condition_variable>
#include <queue>
#include <thread>

struct JobContext
{
	uint32_t thread_index;
};
using JobFunc = std::function<void(JobContext)>;

class ThreadPool
{
public:
	ThreadPool()
	{
	}

	~ThreadPool()
	{
	}

	void start()
	{
		const uint32_t num_threads = std::thread::hardware_concurrency();
		for (uint32_t i = 0; i < num_threads; i++)
		{
			m_threads.at(i) = std::thread(&ThreadPool::thread_loop, this, i);
		}
	}

	void queue_job(const JobFunc& job)
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_Jobs.push(job);
		}

		m_mutex_condition.notify_one();
	}

	void stop()
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_should_terminate = true;
		}

		m_mutex_condition.notify_all();
		for (std::thread& activeThread : m_threads)
		{
			activeThread.join();
		}

		m_threads.clear();
	}

	bool busy()
	{
		bool busy;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			busy = m_Jobs.empty();
		}

		return busy;
	}

	inline size_t get_num_threads() const
	{
		return m_threads.size();
	}

private:
	void thread_loop(uint32_t index)
	{
		while (true)
		{
			JobFunc job;
			{
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				m_mutex_condition.wait(lock, [this]() {
					return !m_Jobs.empty() || m_should_terminate;
				});

				if (m_should_terminate)
				{
					return;
				}

				job = m_Jobs.front();
				m_Jobs.pop();
			}

			job({ index });
		}
	}


private:
	std::mutex m_queue_mutex                  = {};
	std::condition_variable m_mutex_condition = {};
	std::vector<std::thread> m_threads        = {};
	std::queue<JobFunc> m_Jobs                = {};

	bool m_should_terminate = false;
};

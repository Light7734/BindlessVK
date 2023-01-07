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
		for (uint32_t i = 0; i < num_threads; i++) {
			threads.at(i) = std::thread(&ThreadPool::thread_loop, this, i);
		}
	}

	void queue_job(const JobFunc& job)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			jobs.push(job);
		}

		mutex_condition.notify_one();
	}

	void stop()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			should_terminate = true;
		}

		mutex_condition.notify_all();
		for (std::thread& activeThread : threads) {
			activeThread.join();
		}

		threads.clear();
	}

	bool busy()
	{
		bool busy;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			busy = jobs.empty();
		}

		return busy;
	}

	inline size_t get_num_threads() const
	{
		return threads.size();
	}

private:
	void thread_loop(uint32_t index)
	{
		while (true) {
			JobFunc job;
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				mutex_condition.wait(lock, [this]() { return !jobs.empty() || should_terminate; });

				if (should_terminate) {
					return;
				}

				job = jobs.front();
				jobs.pop();
			}

			job({ index });
		}
	}


private:
	std::mutex queue_mutex                  = {};
	std::condition_variable mutex_condition = {};
	std::vector<std::thread> threads        = {};
	std::queue<JobFunc> jobs                = {};

	bool should_terminate = false;
};

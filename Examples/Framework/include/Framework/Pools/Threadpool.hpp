#pragma once

#include "Framework/Common/Common.hpp"

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
	ThreadPool();

	~ThreadPool();

	void start();

	void queue_job(const JobFunc& job);

	void stop();
	bool busy();

	inline size_t get_num_threads() const
	{
		return threads.size();
	}

private:
	void thread_loop(uint32_t index);

private:
	std::mutex queue_mutex                  = {};
	std::condition_variable mutex_condition = {};
	std::vector<std::thread> threads        = {};
	std::queue<JobFunc> jobs                = {};

	bool should_terminate = false;
};

#include "Framework/Pools/Threadpool.hpp"

ThreadPool::ThreadPool()
{
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::start()
{
	const uint32_t num_threads = std::thread::hardware_concurrency();
	for (uint32_t i = 0; i < num_threads; i++) {
		threads.at(i) = std::thread(&ThreadPool::thread_loop, this, i);
	}
}

void ThreadPool::queue_job(const JobFunc& job)
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		jobs.push(job);
	}

	mutex_condition.notify_one();
}

void ThreadPool::stop()
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

bool ThreadPool::busy()
{
	bool busy;
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		busy = jobs.empty();
	}

	return busy;
}

void ThreadPool::thread_loop(uint32_t index)
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

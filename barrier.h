#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

class barrier {

	const size_t threadCount;
	std::atomic<size_t>thrWaiting;
	std::condition_variable wait_;
	std::mutex mutex;

public:
	explicit barrier(size_t n) : threadCount(n) {
		thrWaiting = 0;
	}

	barrier(const barrier &) = delete;
	barrier& operator=(const barrier &) = delete;

	void enter() {
		std::unique_lock<std::mutex> lock(mutex);
		if (thrWaiting.fetch_add(1) >= threadCount - 1) {
			thrWaiting.store(0);
			wait_.notify_all();
		}
		else {
			wait_.wait(lock);
		}
	}
};
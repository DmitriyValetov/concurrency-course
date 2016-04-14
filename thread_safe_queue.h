#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <exception>

template <class Value, class Container = std::deque<Value>>
class thread_safe_queue {

	std::condition_variable c_in;
	std::condition_variable c_out;
	std::mutex mutex_;
	Container queue_;
	std::size_t capacity_;
	std::exception ex;

	bool flag_shutdown;
public:
	thread_safe_queue(std::size_t capacity)
		: capacity_(capacity)
		, flag_shutdown(false) {};

	thread_safe_queue(const thread_safe_queue& queue) = delete;

	void enqueue(Value item) {
		if (flag_shutdown)
			throw ex;
		std::unique_lock<std::mutex> m(mutex_);

		if (full())
			c_out.wait(m, [this] { return queue_.size() < capacity_; });

		queue_.push_back(move(item));
		m.unlock();
		c_in.notify_one();
	}
	void pop(Value& item) {
		std::unique_lock<std::mutex> f(mutex_);
			if (flag_shutdown) {
				c_in.notify_all();
				if (queue_.empty())
					throw ex;
			}
			if (queue_.empty())
				c_in.wait(f, [this] { return !queue_.empty(); });

			item = move(queue_.front());
			queue_.pop_front();
			f.unlock();
			c_out.notify_one();
		}
	bool full()
	{
		if (queue_.size() == capacity_)
			return true;
		else
			return false;
	}
	void shutdown(){
		flag_shutdown = true;
	}
};
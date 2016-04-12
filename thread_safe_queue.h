#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdlib>

using namespace std;

template <class Value, class Container = std::deque<Value>>
class thread_safe_queue {
	deque<Value> impl;
	mutable mutex m;
	condition_variable c_in;
	condition_variable c_out;
	bool flag_shutdown;
	size_t count;
	thread_safe_queue(const thread_safe_queue&) = delete;
	void operator=(const thread_safe_queue&) = delete;

public:
	thread_safe_queue(size_t capacity) {
		impl.resize(capacity);
		flag_shutdown = false;
		count = 0;
	}

	void shutdown() {
		flag_shutdown = true;
	}

	void pop(Value& t){
		unique_lock<std::mutex> l(m);
		try {
			if (flag_shutdown) {
				c_in.notify_all();
				if (impl.empty())
					throw "error";
			}
			if (impl.empty()) {
				auto not_empty = [this] { return !impl.empty(); };
				c_out.wait(l, not_empty);
			}
			count--;
			c_in.notify_one();
			t = impl.front();
			impl.pop_back();
		}
		catch (string){
			exit(1);
		}
	}
	void enqueue(const Value& t){
		unique_lock<mutex> f(m);
		if (full()) {
			auto not_overloaded = [this] { return !full(); };
			c_in.wait(f, not_overloaded);
		}
		impl.push_back(t);
		count++;
		c_out.notify_one();
	}
	bool full() {
		return (count == impl.size());
	}
	bool empty() {
		return impl.empty();
	}

};
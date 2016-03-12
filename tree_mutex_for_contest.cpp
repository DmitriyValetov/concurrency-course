#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <array>
#include <mutex>
#include <stack>

size_t upper_power_of_two(size_t v) {
	size_t size = 1;
	while (size < v)
		size = size * 2;
	return size;
}

class tree_mutex
{
	class peterson_mutex {
	public:
		peterson_mutex() {
			want[0].store(false);
			want[1].store(false);
			victim.store(0);
		}

		void lock(int t) {
			want[t].store(true);
			victim.store(t);
			while (want[1 - t].load() && victim.load() == t) {
				// wait
				std::this_thread::yield();
			}
		}

		void unlock(int t) {
			want[t].store(false);
		}

	private:
		std::array<std::atomic<bool>, 2> want;
		std::atomic<int> victim;
	};

	size_t num_threads;
	std::vector <peterson_mutex> _mutexes;

	size_t getMutexNumberByThread(size_t threadIndex) {
		return (upper_power_of_two(num_threads) / 2 - 1 + threadIndex / 2);
	}

	std::stack<size_t> getPath(std::size_t threadIndex) {
		std::stack<size_t> result;
		std::size_t mutexIndex = getMutexNumberByThread(threadIndex);
		while (mutexIndex) {
			result.push(mutexIndex);
			mutexIndex = _getParentIndex(mutexIndex);
		}
		result.push(0);
		return result;
	}

	size_t _getParentIndex(size_t curr)
	{
		return (curr - 1) / 2;
	}
	size_t _getLeftChildIndex(size_t curr)
	{
		return 2 * curr + 1;
	}
	size_t _getRightChildIndex(size_t curr)
	{
		return 2 * curr + 2;
	}

	void _unlock(size_t mutexIndex, bool isLeftChild) {
		_mutexes[mutexIndex].unlock(isLeftChild);
		if (mutexIndex != 0)
			_unlock(_getParentIndex(mutexIndex), mutexIndex % 2);
	}
	void _lock(size_t mutexIndex, bool isLeftChild) {
		_mutexes[mutexIndex].lock(isLeftChild);
		if (mutexIndex != 0)
			_lock(_getParentIndex(mutexIndex), mutexIndex % 2);
	}
public:
	tree_mutex(size_t _num_threads)
		: num_threads(_num_threads)
		, _mutexes(2 * upper_power_of_two(_num_threads / 2) + 10)
	{
	};
	void lock(std::size_t thread_index)
	{
		if (num_threads == 1)
			_lock(thread_index, thread_index % 2);
		else
			_lock(getMutexNumberByThread(thread_index), thread_index % 2);
	}
	void unlock(std::size_t thread_index)
	{
		if (num_threads == 1)
			_unlock(thread_index, thread_index % 2);
		else {
			std::stack<size_t> path = getPath(thread_index);
			size_t myMutex = getMutexNumberByThread(thread_index);
			size_t currMutex = path.top();
			while (myMutex != path.top()) {
				currMutex = path.top();
				path.pop();
				_mutexes[currMutex].unlock(path.top() % 2);
			}
			_mutexes[myMutex].unlock(thread_index % 2);
			path.pop();
		}
	}
};

int main()
{
	return 0;
}
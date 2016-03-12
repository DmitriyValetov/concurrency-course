#include <thread>
#include <iostream>
#include <vector>
#include <map> 
#include <exception>
#include <mutex>
#include <atomic>
#include <array>

using std::cout;
using std::endl;
using std::vector;
using std::thread;

void SayHello() {
    for (int i = 0; i < 10; i++)
        cout << i << ": Hello, World" << endl;
}

int FastPower(int base, int power) {
    int currentPower = 0;
    while (1 << currentPower <= power) {
        ++currentPower;
    }

    int result = 1;
    while (currentPower >= 0) {
        if (power & (1 << currentPower)) {
            result *= result * base;
        } else {
            result *= result;
        }
        --currentPower;
    }

    return result;
}

template<int Power>
class BinaryConcurencyTree {
private:
    class PetersonMutex {
    public:
        PetersonMutex() {
            want[0].store(false);
            want[1].store(false);
            victim.store(0);
        }

        void lock(int t) {
            want[t].store(true);
            victim.store(t);
            while (want[1 - t].load() && victim.load() == t) {
                // wait
            }
        }
            
        void unlock(int t) {
            want[t].store(false);
        }
    private:
        std::array<std::atomic<bool>, 2> want;
        std::atomic<int> victim;
    };

    struct BindedMutexIndex {
        size_t MutexIndex;
        bool IsLeftChild;
    };

    int32_t _threadNumber;
    vector<PetersonMutex> _mutexes;
    std::map<thread::id, BindedMutexIndex> _threadToMutex;
    mutable std::mutex _safeAssigningMutex;

    int32_t _getParentIndex(int32_t curr) const {
        return (curr - 1) / 2;
    }
    int32_t _getLeftChildIndex(int32_t curr) const{
        return 2 * curr + 1;
    }
    int32_t _getRightChildIndex(int32_t curr) const {
        return 2 * curr + 2;
    }

    void _lock(int32_t mutexIndex, bool isLeftChild) {
        _mutexes[mutexIndex].lock(isLeftChild);
        if (mutexIndex != 0)
            _lock(_getParentIndex(mutexIndex), mutexIndex % 2);
    }

    void _unlock(int32_t mutexIndex, bool isLeftChild) {
        _mutexes[mutexIndex].unlock(isLeftChild);
        if (mutexIndex != 0)
            _unlock(_getParentIndex(mutexIndex), mutexIndex % 2);
    }
public: 
    BinaryConcurencyTree()
        : _threadNumber(FastPower(2, Power))
        , _mutexes((1 << Power) - 1)
        , _threadToMutex() 
    {
    };

    int32_t getThreadNumber() {
        return _threadMunber;
    }

    void lock() {
        auto id = std::this_thread::get_id();
        if (_threadToMutex.count(id) == 0) {
            _safeAssigningMutex.lock();
            _threadToMutex[id] = { 
                (_threadToMutex.size() / 2) + (1 << _threadNumber / 4) - 1,
                (bool)((_threadToMutex.size() + 1) % 2)
            };
            _safeAssigningMutex.unlock();
        }

        _lock(_threadToMutex[id].MutexIndex, _threadToMutex[id].IsLeftChild);
    }
    void unlock() {
        auto binded = _threadToMutex[std::this_thread::get_id()];
        _unlock(binded.MutexIndex, binded.IsLeftChild);
    }

};

const size_t threadsNumberLog = 2;
int main() {
    try {
        BinaryConcurencyTree<threadsNumberLog> bct;

        int threadsNumber = FastPower(2, threadsNumberLog);
        vector<thread> threads(threadsNumber);
        for (size_t i = 0; i < threadsNumber; ++i) {
            threads[i] = thread([&bct]() {
                bct.lock();
                cout << std::this_thread::get_id() << " starts job" << endl;
                for (int i = 0; i < 10; i++)
                    cout << std::this_thread::get_id() << " - " << i << ": Hello, World" << endl;
                cout << std::this_thread::get_id() << " finished job" << endl;
                bct.unlock();
            });
        }

        std::for_each(threads.begin(), threads.end(), std::mem_fn(&thread::join));
        cout << endl;
    }
    catch (std::exception ex ) {
        cout << ex.what();
    }
    return 0;
}

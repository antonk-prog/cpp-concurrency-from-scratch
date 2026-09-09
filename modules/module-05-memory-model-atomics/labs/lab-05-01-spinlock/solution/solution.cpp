#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class spinlock_mutex {
    std::atomic_flag flag;
public:
    spinlock_mutex() : flag(ATOMIC_FLAG_INIT) {}

    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // крутимся, пока флаг уже установлен другим потоком
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

int main() {
    const int threads_count = 4;
    const int increments = 100000;

    spinlock_mutex m;
    long long counter = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < threads_count; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < increments; ++j) {
                m.lock();
                ++counter;
                m.unlock();
            }
        });
    }

    for (auto& t : threads) t.join();

    const long long expected = 1LL * threads_count * increments;
    std::cout << "counter = " << counter
              << ", expected = " << expected << "\n";

    if (counter == expected) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cout << "FAIL: гонка за данные (counter < expected)\n";
        return 1;
    }
}

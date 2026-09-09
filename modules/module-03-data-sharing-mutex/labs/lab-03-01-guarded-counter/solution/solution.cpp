#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class Counter {
public:
    void increment() {
        std::lock_guard<std::mutex> guard(m_);
        ++value_;
    }

    int get() const {
        std::lock_guard<std::mutex> guard(m_);
        return value_;
    }

private:
    int value_ = 0;
    mutable std::mutex m_;
};

int main() {
    const int threads_count = 4;
    const int iterations = 100000;

    Counter counter;
    std::vector<std::thread> threads;
    for (int t = 0; t < threads_count; ++t) {
        threads.emplace_back([&counter, iterations] {
            for (int i = 0; i < iterations; ++i) {
                counter.increment();
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Final counter: " << counter.get() << "\n";
    return 0;
}
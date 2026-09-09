#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class atomic_counter {
    std::atomic<int> value_{0};
public:
    void increment() {
        int expected = value_.load();
        while (!value_.compare_exchange_weak(expected, expected + 1)) {
            // expected обновлён неудачной попыткой; пробуем снова
        }
    }

    void decrement() {
        int expected = value_.load();
        while (!value_.compare_exchange_weak(expected, expected - 1)) {
            // expected обновлён неудачной попыткой; пробуем снова
        }
    }

    int value() const {
        return value_.load();
    }
};

int main() {
    const int threads_count = 8;
    const int iterations = 100000;

    atomic_counter counter;

    std::vector<std::thread> threads;
    for (int i = 0; i < threads_count; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < iterations; ++j) {
                counter.increment();
                counter.decrement();
            }
        });
    }

    for (auto& t : threads) t.join();

    const int final_value = counter.value();
    std::cout << "final value = " << final_value << "\n";

    if (final_value == 0) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cout << "FAIL: счётчик не сошёлся к 0\n";
        return 1;
    }
}

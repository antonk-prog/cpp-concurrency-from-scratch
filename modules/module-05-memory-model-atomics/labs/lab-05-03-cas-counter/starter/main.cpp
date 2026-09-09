#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// TODO: реализуй атомарный счётчик через CAS-цикл.
//
// increment():  цикл compare_exchange_weak: прочитай текущее значение,
//               предложи текущее+1, повтори, пока CAS не вернёт true.
// decrement():  аналогично, но предложи текущее-1.
// value():      верни load().
//
// ЗАПРЕЩЕНО использовать fetch_add / fetch_sub / ++ / -- / += / -=.
// По умолчанию используется std::memory_order_seq_cst.

class atomic_counter {
    std::atomic<int> value_{0};
public:
    void increment() {
        // TODO
    }

    void decrement() {
        // TODO
    }

    int value() const {
        // TODO
        return 0;
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

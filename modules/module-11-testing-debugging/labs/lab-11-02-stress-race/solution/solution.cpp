#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// ИСПРАВЛЕНИЕ: counter — std::atomic<int>. Атомарные обновления исключают
// гонку и потерю значений; relaxed достаточно (порядок между потоками не нужен).
std::atomic<int> counter{0};

int main() {
    const int threads = 4;
    const int per_thread = 100000;
    const int repeats = 200;

    bool all_ok = true;
    for (int r = 0; r < repeats; ++r) {
        counter = 0;

        std::vector<std::thread> ts;
        for (int t = 0; t < threads; ++t) {
            ts.emplace_back([&] {
                for (int i = 0; i < per_thread; ++i) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& t : ts) t.join();

        if (counter.load() != threads * per_thread) {
            all_ok = false;
        }
    }

    std::cout << "result = " << (all_ok ? "OK" : "FAIL") << "\n";
    return all_ok ? 0 : 1;
}
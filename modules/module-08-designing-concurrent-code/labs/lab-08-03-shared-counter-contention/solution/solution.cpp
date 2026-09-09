#include <atomic>
#include <chrono>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

using steady = std::chrono::steady_clock;

long long shared_counter(int threads, long n) {
    std::atomic<long> counter{0};
    long long best = -1;
    for (int rep = 0; rep < 3; ++rep) {
        auto t0 = steady::now();
        std::vector<std::thread> ts;
        for (int i = 0; i < threads; ++i) {
            ts.emplace_back([&] {
                for (long k = 0; k < n; ++k)
                    counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : ts) t.join();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           steady::now() - t0).count();
        best = (best < 0) ? ms : std::min(best, ms);
    }
    return best;
}

// КАЖДЫЙ поток увеличивает СВОЙ счётчик, выровненный по отдельной строке кэша.
// Перебрасывания строки между ядрами нет — конкуренции нет.
long long own_counters(int threads, long n) {
    struct item {
        alignas(std::hardware_destructive_interference_size) std::atomic<long> v;
    };
    std::vector<item> counters(threads);
    long long best = -1;
    for (int rep = 0; rep < 3; ++rep) {
        auto t0 = steady::now();
        std::vector<std::thread> ts;
        for (int i = 0; i < threads; ++i) {
            ts.emplace_back([&, i] {
                for (long k = 0; k < n; ++k)
                    counters[i].v.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : ts) t.join();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           steady::now() - t0).count();
        best = (best < 0) ? ms : std::min(best, ms);
    }
    return best;
}

int main() {
    const int threads = 4;
    const long n = 100'000'000;

    long long single = shared_counter(1, threads * n);
    long long shared = shared_counter(threads, n);
    long long own = own_counters(threads, n);

    std::cout << "1 поток, общий счётчик        : " << single << " ms\n";
    std::cout << threads << " потока, общий счётчик        : " << shared << " ms\n";
    std::cout << threads << " потока, свои счётчики        : " << own << " ms\n";
    std::cout << "свои быстрее общего: " << (own < shared ? "да" : "НЕТ") << "\n";
    return 0;
}
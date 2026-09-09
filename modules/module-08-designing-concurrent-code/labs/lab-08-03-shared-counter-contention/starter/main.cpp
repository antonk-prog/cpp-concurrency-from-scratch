#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using steady = std::chrono::steady_clock;

// ВСЕ потоки увеличивают ОДИН общий атомарный счётчик.
// fetch_add — «чтение-изменение-запись»: нужна свежая строка кэша.
// Пока один поток держит её, остальные ждут — строка «курсирует» между ядрами.
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

int main() {
    const int threads = 4;
    const long n = 100'000'000;

    long long single = shared_counter(1, threads * n);  // один поток, вся работа
    long long multi = shared_counter(threads, n);       // threads потоков, та же работа

    std::cout << "1 поток  : " << single << " ms\n";
    std::cout << threads << " потока : " << multi << " ms\n";
    std::cout << "многопоточный быстрее: " << (single > multi ? "да" : "НЕТ") << "\n";
    return 0;
}
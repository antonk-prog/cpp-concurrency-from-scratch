#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kWorkers = 4;
constexpr int kIterations = 2000;

void worker(long long& shared_total, std::mutex& m) {
    for (int i = 0; i < kIterations; ++i) {
        std::unique_lock<std::mutex> lk(m, std::defer_lock); // объект есть, мьютекс свободен

        long long local = 0;
        for (int j = 0; j < 500; ++j) local += j;   // локальная работа БЕЗ блокировки

        lk.lock();                  // блокировка только на время слияния
        shared_total += local;
        lk.unlock();                // сразу сняли
    }
}

int main() {
    long long shared_total = 0;
    std::mutex m;

    std::vector<std::thread> threads;
    for (int t = 0; t < kWorkers; ++t) {
        threads.emplace_back([&] { worker(shared_total, m); });
    }
    for (auto& t : threads) t.join();

    std::cout << "Shared total: " << shared_total << "\n";
    return 0;
}
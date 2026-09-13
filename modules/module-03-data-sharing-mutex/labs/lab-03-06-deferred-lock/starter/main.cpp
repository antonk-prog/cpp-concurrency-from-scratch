// Лаба 3.6. Допиши воркер: unique_lock с defer_lock, явные lock()/unlock().
// Требования — в task.md. Каркас компилируется, но без блокировки итог неверен.
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kWorkers = 4;
constexpr int kIterations = 2000;

void worker(long long& shared_total, std::mutex& m) {
    (void)shared_total;
    (void)m;
    for (int i = 0; i < kIterations; ++i) {
        // TODO 1: создай std::unique_lock<std::mutex> lk(m, std::defer_lock)
        //         ДО локальной работы — мьютекс ещё не заблокирован.

        long long local = 0;
        for (int j = 0; j < 500; ++j) local += j;   // локальная работа

        // TODO 2: lk.lock(), прибавь local к shared_total, затем lk.unlock().
        //         Блокировка держится ТОЛЬКО на время слияния.
    }
}

int main() {
    long long shared_total = 0;
    std::mutex m;

    // TODO 3: запусти kWorkers потоков с worker(shared_total, m),
    //         дождись всех через join().

    std::cout << "Shared total: " << shared_total << "\n";
    return 0;
}
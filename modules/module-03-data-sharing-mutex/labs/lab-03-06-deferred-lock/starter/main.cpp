// Лаба 3.6. Допиши воркер: unique_lock с defer_lock, явные lock()/unlock().
// Требования — в task.md. Здесь только worker.
#include <mutex>

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
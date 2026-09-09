#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Потокобезопасная очередь (модуль 6) — дана.
class thread_safe_queue {
    std::mutex mut;
    std::queue<std::function<void()>> data;
    std::condition_variable cv;
public:
    void push(std::function<void()> f) {
        std::lock_guard<std::mutex> lk(mut);
        data.push(std::move(f));
    }

    bool try_pop(std::function<void()>& f) {
        std::lock_guard<std::mutex> lk(mut);
        if (data.empty()) return false;
        f = std::move(data.front());
        data.pop();
        return true;
    }
};

// RAII-присоединение всех потоков (модуль 8) — дано.
class join_threads {
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& t) : threads(t) {}
    ~join_threads() {
        for (auto& th : threads)
            if (th.joinable()) th.join();
    }
};

// TODO: реализуй пул потоков.
// Порядок полей важен: done и очередь ДО threads, threads ДО joiner.
class thread_pool {
    std::atomic<bool> done;
    thread_safe_queue work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        // TODO: цикл while(!done): try_pop — есть, выполнить; нет — yield().
        // Заглушка: поток завершается сразу.
    }

public:
    thread_pool() : done(false), joiner(threads) {
        unsigned const count = std::thread::hardware_concurrency();
        // TODO: при исключении в цикле — done = true; throw;
        for (unsigned i = 0; i < count; ++i) {
            threads.emplace_back(&thread_pool::worker_thread, this);
        }
    }

    ~thread_pool() {
        done = true;  // TODO: достаточно ли этого? (потоки join'ит joiner)
    }

    template <typename FunctionType>
    void submit(FunctionType f) {
        // TODO: work_queue.push(std::function<void()>(f));
        (void)f;
    }
};

int main() {
    const int total = 20000;
    thread_pool pool;

    std::atomic<int> processed{0};
    std::atomic<long long> sum{0};

    for (int i = 0; i < total; ++i) {
        pool.submit([&, i] {
            processed.fetch_add(1);
            sum.fetch_add(i);
        });
    }

    // Пул в этой лабе не умеет ждать завершения задач, поэтому main
    // опрашивает счётчик с предохранителем от бесконечного ожидания.
    const long max_polls = 100'000'000;
    long polls = 0;
    while (processed.load() != total && polls++ < max_polls) {
        std::this_thread::yield();
    }

    bool done_all = (processed.load() == total);
    bool sum_ok = (sum.load() == (long long)total * (total - 1) / 2);

    std::cout << "processed = " << processed.load() << ", expected = " << total << "\n";
    std::cout << "result = " << (done_all && sum_ok ? "OK" : "FAIL") << "\n";
    return (done_all && sum_ok) ? 0 : 1;
}
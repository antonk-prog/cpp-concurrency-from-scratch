#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

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

class join_threads {
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& t) : threads(t) {}
    ~join_threads() {
        for (auto& th : threads)
            if (th.joinable()) th.join();
    }
};

class thread_pool {
    std::atomic<bool> done;
    thread_safe_queue work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            std::function<void()> task;
            if (work_queue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }

public:
    thread_pool() : done(false), joiner(threads) {
        unsigned const count = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < count; ++i) {
                threads.emplace_back(&thread_pool::worker_thread, this);
            }
        } catch (...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() {
        done = true;
    }

    template <typename FunctionType>
    void submit(FunctionType f) {
        work_queue.push(std::function<void()>(f));
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
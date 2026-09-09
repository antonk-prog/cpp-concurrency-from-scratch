#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Move-only обёртка задачи (лекция 9) — дана.
class function_wrapper {
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() {}
    };
    std::unique_ptr<impl_base> impl;

    template <typename F>
    struct impl_type : impl_base {
        F f;
        impl_type(F&& f_) : f(std::move(f_)) {}
        void call() { f(); }
    };

public:
    function_wrapper() {}
    template <typename F>
    function_wrapper(F&& f) : impl(new impl_type<F>(std::move(f))) {}
    void call() { impl->call(); }

    function_wrapper(function_wrapper&& other) : impl(std::move(other.impl)) {}
    function_wrapper& operator=(function_wrapper&& other) {
        impl = std::move(other.impl);
        return *this;
    }
    function_wrapper(function_wrapper const&) = delete;
    function_wrapper& operator=(function_wrapper const&) = delete;
};

// Потокобезопасная очередь (модуль 6) — дана.
template <typename T>
class thread_safe_queue {
    std::mutex mut;
    std::deque<T> data;
    std::condition_variable cv;
public:
    void push(T value) {
        std::lock_guard<std::mutex> lk(mut);
        data.push_back(std::move(value));
        cv.notify_one();
    }
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data.empty()) return false;
        value = std::move(data.front());
        data.pop_front();
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

// БАГ: пул умеет только submit + рабочий цикл. Нет run_pending_task().
// Рабочий поток, дойдя до .get(), блокируется навсегда — задачи из очереди
// некому выполнять.
class thread_pool {
    std::atomic<bool> done;
    thread_safe_queue<function_wrapper> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            function_wrapper task;
            if (work_queue.try_pop(task)) {
                task.call();
            } else {
                std::this_thread::yield();
            }
        }
    }

public:
    thread_pool() : done(false), joiner(threads) {
        threads.emplace_back(&thread_pool::worker_thread, this);
    }
    ~thread_pool() { done = true; }

    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f) {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res = task.get_future();
        work_queue.push(std::move(task));
        return res;
    }
};

const long long THRESHOLD = 1000;

long long worker(thread_pool& pool, long long lo, long long hi) {
    if (hi - lo <= THRESHOLD) {
        long long s = 0;
        for (long long i = lo; i < hi; ++i) s += i;
        return s;
    }
    long long mid = lo + (hi - lo) / 2;
    auto f = pool.submit([&pool, mid, hi]() { return worker(pool, mid, hi); });
    long long direct = worker(pool, lo, mid);
    return direct + f.get();   // БАГ: ожидание без выполнения задач из очереди
}

int main() {
    thread_pool pool;
    const long long N = 1LL << 20;

    auto result = pool.submit([&pool]() { return worker(pool, 0LL, N); });

    if (result.wait_for(std::chrono::seconds(10)) == std::future_status::ready) {
        long long sum = result.get();
        long long expected = (long long)N * (N - 1) / 2;
        std::cout << "sum = " << sum << "\n";
        std::cout << "result = " << (sum == expected ? "OK" : "FAIL") << "\n";
        return (sum == expected) ? 0 : 1;
    } else {
        std::cout << "DEADLOCK: задача ждёт задачу, а рабочий поток занят\n";
        // Выходим БЕЗ разрушения пула: рабочий поток заблокирован в get(),
        // деструктор пула (join_threads) присоединить его не сможет —
        // он тоже зависнет. Это часть урока про deadlock.
        std::_Exit(0);
    }
}
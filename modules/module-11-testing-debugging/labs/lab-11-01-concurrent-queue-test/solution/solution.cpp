#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>

class threadsafe_queue {
    mutable std::mutex mut;
    std::queue<int> data;
    std::condition_variable cv;
public:
    void push(int value) {
        std::lock_guard<std::mutex> lk(mut);
        data.push(value);
        cv.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [this] { return !data.empty(); });
        int value = data.front();
        data.pop();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data.empty();
    }
};

bool test_concurrent_push_and_pop_on_empty_queue() {
    threadsafe_queue q;

    std::promise<void> go, push_ready, pop_ready;
    std::shared_future<void> ready(go.get_future());

    std::future<void> push_done;
    std::future<int> pop_done;

    bool ok = true;

    try {
        push_done = std::async(std::launch::async, [&q, ready, &push_ready] {
            push_ready.set_value();
            ready.wait();
            q.push(42);
        });
        pop_done = std::async(std::launch::async, [&q, ready, &pop_ready] {
            pop_ready.set_value();
            ready.wait();
            return q.pop();
        });

        push_ready.get_future().wait();
        pop_ready.get_future().wait();
        go.set_value();

        push_done.get();
        ok = ok && (pop_done.get() == 42);
        ok = ok && q.empty();
    } catch (...) {
        go.set_value();
        throw;
    }

    return ok;
}

int main() {
    bool ok = test_concurrent_push_and_pop_on_empty_queue();
    std::cout << "result = " << (ok ? "OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
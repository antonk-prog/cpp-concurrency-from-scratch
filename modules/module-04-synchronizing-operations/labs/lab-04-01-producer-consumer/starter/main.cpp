#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class threadsafe_queue {
public:
    // TODO 1: push(value) — lock_guard, data_queue_.push(value),
    // затем data_cond_.notify_one() ПОСЛЕ снятия блокировки.
    void push(int value) { (void)value; }

    // TODO 2: wait_and_pop(value) — unique_lock, data_cond_.wait(lk, предикат
    // !data_queue_.empty()), затем value = front(); pop().
    void wait_and_pop(int& value) { (void)value; }

    // TODO 3: try_pop(value) — lock_guard; пустая очередь → false;
    // иначе value = front(); pop(); вернуть true.
    bool try_pop(int& value) { (void)value; return false; }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut_);
        return data_queue_.empty();
    }

private:
    std::queue<int> data_queue_;
    mutable std::mutex mut_;
    std::condition_variable data_cond_;
};

int main() {
    // Раскомментируй после дописывания методов:
    //
    // const int n = 100;
    // threadsafe_queue q;
    //
    // std::thread producer([&q, n] {
    //     for (int i = 1; i <= n; ++i) q.push(i);
    // });
    //
    // int sum = 0;
    // std::thread consumer([&q, &sum, n] {
    //     int value = 0;
    //     for (int i = 0; i < n; ++i) {
    //         q.wait_and_pop(value);
    //         sum += value;
    //     }
    // });
    //
    // producer.join();
    // consumer.join();
    // std::cout << "Sum = " << sum << "\n";
    return 0;
}
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class threadsafe_queue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lk(mut_);
            data_queue_.push(value);
        }
        data_cond_.notify_one();   // после разблокировки мьютекса
    }

    void wait_and_pop(int& value) {
        std::unique_lock<std::mutex> lk(mut_);
        data_cond_.wait(lk, [this] { return !data_queue_.empty(); });
        value = data_queue_.front();
        data_queue_.pop();
    }

    bool try_pop(int& value) {
        std::lock_guard<std::mutex> lk(mut_);
        if (data_queue_.empty()) return false;
        value = data_queue_.front();
        data_queue_.pop();
        return true;
    }

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
    const int n = 100;
    threadsafe_queue q;

    std::thread producer([&q, n] {
        for (int i = 1; i <= n; ++i) q.push(i);
    });

    int sum = 0;
    std::thread consumer([&q, &sum, n] {
        int value = 0;
        for (int i = 0; i < n; ++i) {
            q.wait_and_pop(value);
            sum += value;
        }
    });

    producer.join();
    consumer.join();
    std::cout << "Sum = " << sum << "\n";
    return 0;
}
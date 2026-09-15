#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class threadsafe_queue {
public:
    // TODO 1: положить значение в очередь и дать знать ждущему потребителю.
    void push(int value) { (void)value; }

    // TODO 2: если очередь пуста — дождаться появления элемента, затем извлечь его.
    void wait_and_pop(int& value) { (void)value; }

    // TODO 3: не дожидаясь, извлечь элемент; если очередь пуста — вернуть false.
    bool try_pop(int& value) { (void)value; return false; }

private:
    std::queue<int> data_queue_;
    std::mutex mut_;
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

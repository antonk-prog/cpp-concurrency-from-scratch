#include <iostream>
#include <memory>
#include <thread>

void incrementAndSet(int step, int& target) {
    target += step;
}

void consume(std::unique_ptr<int> p) {
    if (p) {
        std::cout << "Consumed value: " << *p << "\n";
    }
}

int main() {
    int counter = 0;
    const int step = 5;

    std::thread t1(incrementAndSet, step, std::ref(counter));
    t1.join();
    std::cout << "Counter after thread 1: " << counter << "\n";

    auto p = std::make_unique<int>(99);
    std::thread t2(consume, std::move(p));
    t2.join();

    std::cout << "Pointer in main is now null: " << (p == nullptr ? "true" : "false") << "\n";
    return 0;
}

#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

void compute_data(std::vector<int>& buffer) {
    buffer.resize(100000);
    std::iota(buffer.begin(), buffer.end(), 1);
}

int main() {
    std::vector<int> buffer;
    std::promise<void> ready;
    auto future = ready.get_future();

    std::thread producer([&] {
        try {
            compute_data(buffer);       // сначала готовим данные…
            ready.set_value();          // …потом сигнализируем о готовности
        } catch (...) {
            ready.set_exception(std::current_exception());
        }
    });

    std::thread consumer([&] {
        future.get();                   // после get() все записи до set_value видны
        long long sum = 0;
        for (int v : buffer) {
            sum += v;
        }
        long long expected =
            static_cast<long long>(100000) * (100000 + 1) / 2;
        std::cout << (sum == expected ? "OK" : "BAD") << "\n";
    });

    producer.join();
    consumer.join();
    return 0;
}
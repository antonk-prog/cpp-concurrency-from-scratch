#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    const size_t n = 1000000;
    std::vector<int> buffer;
    std::promise<void> ready;
    auto future = ready.get_future();

    // Потребитель создаём ПЕРВЫМ: он сразу встаёт в get() и по сигналу
    // мгновенно начинает читать, пока поставщик ещё заполняет буфер.
    std::thread consumer([&] {
        future.get();
        long long sum = 0;
        for (int v : buffer) {
            sum += v;
        }
        long long expected =
            static_cast<long long>(n) * (n + 1) / 2;
        std::cout << (sum == expected ? "OK" : "BAD") << "\n";
    });

    // Поток-поставщик: выделяет память, сигнализирует о готовности и ЗАТЕМ
    // заполняет буфер элементами.
    std::thread producer([&] {
        try {
            buffer.resize(n);            // память выделена — буфер непуст
            // ДЕФЕКТ 1: сигнал раньше, чем данные готовы — потребитель начнёт
            // читать элементы buffer, пока мы их ещё записываем → гонка
            ready.set_value();
            for (size_t i = 0; i < n; ++i) {
                buffer[i] = static_cast<int>(i + 1);
            }
        } catch (...) {
            // ДЕФЕКТ 2: promise не установлен → broken_promise у ожидающего
        }
    });

    consumer.join();
    producer.join();
    return 0;
}
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    const int n = 1000000;
    std::vector<int> data(n);        // неатомарные данные
    std::atomic<bool> ready(false);  // флаг готовности
    std::atomic<bool> ok(false);     // результат проверки читателя

    std::thread writer([&] {
        for (int i = 0; i < n; ++i) {
            data[i] = i + 1;
            if (i == 0) {
                // TODO: это НЕВЕРНО — флаг выставляется до завершения записи,
                // а relaxed к тому же не упорядочивает данные.
                ready.store(true, std::memory_order_relaxed);
            }
        }
    });

    std::thread reader([&] {
        while (!ready.load(std::memory_order_relaxed)) {
            // ждём флаг, но из-за relaxed данные могут быть ещё не готовы
        }
        long long sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += data[i];          // читаем, пока writer, возможно, ещё пишет
        }
        const long long expected = 1LL * n * (n + 1) / 2;
        ok.store(sum == expected, std::memory_order_relaxed);
    });

    writer.join();
    reader.join();

    if (ok.load(std::memory_order_relaxed)) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cout << "FAIL: данные прочитаны неверно (гонка за данные)\n";
        return 1;
    }
}

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
            data[i] = i + 1;         // сначала пишем все данные
        }
        // только после полной записи выставляем флаг как освобождение
        ready.store(true, std::memory_order_release);
    });

    std::thread reader([&] {
        while (!ready.load(std::memory_order_acquire)) {
            // ждём флаг захватом; данные гарантированно готовы
        }
        long long sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += data[i];
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

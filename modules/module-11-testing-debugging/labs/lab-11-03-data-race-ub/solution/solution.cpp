#include <atomic>
#include <iostream>
#include <thread>

// ИСПРАВЛЕННАЯ версия: value — std::atomic<int>. Гонки нет (UB уходит),
// результат определён (хотя какая запись «победит», зависит от порядка —
// теперь это корректная гонка без UB).
std::atomic<int> value{0};

int main() {
    std::thread a([] {
        for (int i = 1; i <= 3; ++i) {
            value = i;
        }
    });
    std::thread b([] {
        for (int i = 100; i <= 102; ++i) {
            value = i;
        }
    });
    a.join();
    b.join();

    std::cout << "value = " << value.load() << "\n";
    std::cout << "стартер (int) — UB: вывод недетерминирован;\n";
    std::cout << "здесь value атомарная — гонки нет, вывод определён\n";
    return 0;
}
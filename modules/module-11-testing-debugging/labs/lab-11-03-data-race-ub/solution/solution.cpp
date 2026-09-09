#include <iostream>
#include <thread>

// То же, что в стартере: гонка за данные (UB). Программа печатает значение —
// вывод недетерминирован, конкретное число предсказать нельзя.
int value = 0;

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

    std::cout << "value = " << value << "\n";
    std::cout << "вывод недетерминирован: гонка за данные (UB)\n";
    return 0;
}
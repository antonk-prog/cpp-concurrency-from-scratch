#include <iostream>
#include <thread>

// Гонка за данные: value пишут два потока без синхронизации, читает main.
// Это UB — вывод недетерминирован.
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
    return 0;
}
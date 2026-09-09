#include <iostream>
#include <thread>

// Сумма целых от 1 до n включительно. Результат — в sum (передаётся по ссылке).
void computeSum(int n, int& sum) {
    sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += i;
    }
}

// Сколько чисел от 1 до n делятся нацело на 7. Результат — в count.
void countMultiplesOfSeven(int n, int& count) {
    count = 0;
    for (int i = 1; i <= n; ++i) {
        if (i % 7 == 0) {
            ++count;
        }
    }
}

int main() {
    const int n = 100;
    int sum = 0;
    int count = 0;

    // TODO: запусти два потока — computeSum и countMultiplesOfSeven.
    // Каждому передай n и ссылку на свою переменную результата.

    // TODO: дождись завершения обоих потоков (join()).

    std::cout << "Sum 1.." << n << " = " << sum << "\n";
    std::cout << "Multiples of 7 in 1.." << n << " = " << count << "\n";
    return 0;
}

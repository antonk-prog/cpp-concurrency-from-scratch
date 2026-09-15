// Лаба 4.4. Напиши программу с std::packaged_task с нуля.
// Требования — в task.md. Этот каркас компилируется.
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

int square(int x) {
    return x * x;
}

int checked_divide(int a, int b) {
    if (b == 0) throw std::runtime_error("division by zero");
    return a / b;
}

int main() {
    // Напиши: упакуй обычную функцию в std::packaged_task, получи фьючерс через
    // get_future(), выполни задачу в отдельном потоке и забери результат через
    // get(). Затем повтори то же для функции, которая бросает исключение, и
    // убедись, что исключение выходит из get().
    return 0;
}

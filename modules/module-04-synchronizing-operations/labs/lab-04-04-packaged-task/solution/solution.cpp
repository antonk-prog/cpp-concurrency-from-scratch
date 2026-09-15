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
    std::packaged_task<int(int)> square_task(square);
    std::future<int> square_future = square_task.get_future();
    std::thread squarer(std::move(square_task), 7);
    std::cout << "square(7) = " << square_future.get() << "\n";
    squarer.join();

    std::packaged_task<int(int, int)> divide_task(checked_divide);
    std::future<int> divide_future = divide_task.get_future();
    std::thread divider(std::move(divide_task), 1, 0);
    try {
        int quotient = divide_future.get();
        std::cout << "1 / 0 = " << quotient << "\n";
    } catch (std::exception const& e) {
        std::cout << "caught: " << e.what() << "\n";
    }
    divider.join();

    return 0;
}

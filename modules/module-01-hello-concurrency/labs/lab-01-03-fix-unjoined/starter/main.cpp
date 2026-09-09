#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

void doWork(int value) {
    std::cout << "Обработка данных: " << value << "\n";
}

void processData(int value) {
    // Проверка входных данных: при неверном значении бросаем исключение.
    if (value < 0) {
        throw std::runtime_error("неверный параметр");
    }
}

int main(int argc, char* argv[]) {
    int value = 0;
    if (argc > 1) {
        value = std::stoi(argv[1]);
    }

    std::thread worker(doWork, value);
    processData(value);   // здесь может быть брошено исключение
    worker.join();        // если исключение брошено, сюда мы не дойдём
    return 0;
}

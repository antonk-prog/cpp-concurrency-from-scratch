#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t) : thread_(t) {}
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
    ~ThreadGuard() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::thread& thread_;
};

void doWork(int value) {
    std::cout << "Обработка данных: " << value << "\n";
}

void processData(int value) {
    if (value < 0) {
        throw std::runtime_error("неверный параметр");
    }
}

int main(int argc, char* argv[]) {
    try {
        int value = 0;
        if (argc > 1) {
            value = std::stoi(argv[1]);
        }

        std::thread worker(doWork, value);
        ThreadGuard guard(worker);   // при любом исходе join() в деструкторе
        processData(value);
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}

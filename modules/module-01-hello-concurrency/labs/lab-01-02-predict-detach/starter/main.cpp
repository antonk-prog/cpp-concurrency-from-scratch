#include <iostream>
#include <thread>

void background() {
    std::cout << "Фоновая задача отработала\n";
}

int main() {
    std::thread t(background);
    t.detach();
    std::cout << "Главный поток завершается\n";
    return 0;
}

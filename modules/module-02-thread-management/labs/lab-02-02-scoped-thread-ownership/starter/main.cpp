#include <iostream>
#include <thread>

void doWork(int value) {
    std::cout << "Worker processed: " << value << "\n";
}

class scoped_thread {
public:
    // TODO 1: конструктор, принимающий std::thread и владеющий им
    //         (перемести поток в член t). Объяви explicit.

    // TODO 2: запрети копирование (= delete).

    // TODO 3: деструктор — join(), если t.joinable().

private:
    std::thread t;
};

// TODO 4: функция makeWorker(int), возвращающая std::thread,
//         который выполняет doWork с переданным значением.

int main() {
    // Раскомментируй строку ниже после того, как допишешь класс и makeWorker:
    // scoped_thread st(makeWorker(7));
    return 0;
}
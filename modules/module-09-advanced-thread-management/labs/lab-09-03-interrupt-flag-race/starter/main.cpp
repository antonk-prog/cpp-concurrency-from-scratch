#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class thread_interrupted {};

// БАГ: flag — обычный bool. Его пишет прерывающий поток (set), а читают
// прерываемые (is_set) в цикле. Чтение и запись из разных потоков без
// атомарности — гонка за данные (UB).
//
// TODO: исправь — сделай flag std::atomic<bool> и используй store()/load().
class interrupt_flag {
public:
    void set() { flag = true; }
    bool is_set() const { return flag; }

private:
    bool flag = false;
};

// В лекции у каждого потока свой thread_local флаг. Здесь — один общий флаг:
// прерываем сразу все потоки (как при завершении приложения). Гонка от этого
// не меняется. (Для проверки TSan флаг должен лежать в обычной памяти:
// доступ через thread_local он не инструментирует.)
interrupt_flag global_flag;

std::atomic<int> work{0};
std::atomic<int> finished{0};

void background() {
    try {
        while (true) {
            if (global_flag.is_set()) {
                throw thread_interrupted();
            }
            work.fetch_add(1, std::memory_order_relaxed);
        }
    } catch (thread_interrupted const&) {
        // поток остановлен по прерыванию
    }
    finished.fetch_add(1, std::memory_order_relaxed);
}

int main() {
    const int threads = 4;
    const int threshold = 10'000'000;

    std::vector<std::thread> ts;
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back(background);
    }

    // ждём, пока фоновые потоки наработают, и начинаем прерывание
    while (work.load(std::memory_order_relaxed) < threshold) {
        std::this_thread::yield();
    }
    // сигнал подаём повторно, пока все потоки не подтвердят остановку
    while (finished.load(std::memory_order_relaxed) < threads) {
        for (int k = 0; k < 5; ++k) {
            global_flag.set();
        }
        std::this_thread::yield();
    }
    for (auto& t : ts) {
        t.join();
    }

    bool ok = (work.load(std::memory_order_relaxed) >= threshold) &&
              (finished.load() == threads);
    std::cout << "work = " << work.load() << ", finished = " << finished.load() << "\n";
    std::cout << "result = " << (ok ? "OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
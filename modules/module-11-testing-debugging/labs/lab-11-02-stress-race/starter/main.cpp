#include <iostream>
#include <thread>
#include <vector>

// БАГ: общий счётчик — обычный int. Его изменяют все потоки (++counter),
// читает главный поток после join. Чтение/запись из разных потоков без
// синхронизации — гонка за данные (UB); типичный симптом — потерянные
// обновления (итог меньше ожидаемого).
//
// TODO: исправь — сделай counter std::atomic<int> и используй fetch_add/load.
int counter = 0;

int main() {
    const int threads = 4;
    const int per_thread = 100000;
    const int repeats = 200;

    bool all_ok = true;
    for (int r = 0; r < repeats; ++r) {
        counter = 0;

        std::vector<std::thread> ts;
        for (int t = 0; t < threads; ++t) {
            ts.emplace_back([&] {
                for (int i = 0; i < per_thread; ++i) {
                    ++counter;
                }
            });
        }
        for (auto& t : ts) t.join();

        if (counter != threads * per_thread) {
            all_ok = false;
        }
    }

    std::cout << "result = " << (all_ok ? "OK" : "FAIL") << "\n";
    return all_ok ? 0 : 1;
}
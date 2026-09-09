#include <algorithm>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

// RAII-обёртка: присоединяет все потоки в деструкторе.
// TODO: реализуй класс join_threads — хранит ссылку на вектор std::thread,
//       в деструкторе join() всех joinable() потоков.
class join_threads {
    // ...
public:
    // explicit join_threads(std::vector<std::thread>& threads);
    // ~join_threads();
};

// Параллельный for_each.
// TODO: реализуй функцию.
template <typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f) {
    // 1. length = std::distance(first, last); если 0 — вернуться.
    // 2. num_threads = min(hardware_concurrency(), (length+24)/25).
    // 3. block_size = length / num_threads.
    // 4. futures(num_threads-1), threads(num_threads-1), join_threads joiner.
    // 5. Цикл: блок в packaged_task<void(void)> + новый поток.
    // 6. Последний блок — в текущем потоке.
    // 7. futures[i].get() для повторной выдачи исключений.
    (void)first;
    (void)last;
    (void)f;
}

int main() {
    const int n = 100000;
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);

    parallel_for_each(data.begin(), data.end(), [](int& x) { x *= 2; });

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        if (data[i] != 2 * i) {
            ok = false;
            break;
        }
    }

    std::cout << "result = " << (ok ? "OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
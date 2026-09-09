#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class thread_interrupted {};

// ИСПРАВЛЕНИЕ: flag — std::atomic<bool>. Запись из прерывающего потока и
// чтение из прерываемого больше не гонка; seq_cst достаточно.
class interrupt_flag {
public:
    void set() { flag.store(true); }
    bool is_set() const { return flag.load(); }

private:
    std::atomic<bool> flag{false};
};

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

    while (work.load(std::memory_order_relaxed) < threshold) {
        std::this_thread::yield();
    }
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
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

constexpr int kCount = 50;
constexpr int kConsumers = 4;

int items[kCount];
std::atomic<int> count{0};
bool processed[kCount] = {false};

void publish() {
    for (int i = 0; i < kCount; ++i) {
        items[i] = i * 2;
    }
    // TODO: опубликуй партию: count.store(kCount, std::memory_order_release)
}

int claim() {
    // TODO: забери элемент: верни count.fetch_sub(1, std::memory_order_acquire)
    return 0;
}

void consumer() {
    while (true) {
        int v = claim();
        if (v <= 0) break;
        int idx = v - 1;
        if (items[idx] == idx * 2) {
            processed[idx] = true;
        }
    }
}

int main() {
    std::thread prod(publish);
    prod.join();

    std::vector<std::thread> cs;
    for (int i = 0; i < kConsumers; ++i) {
        cs.emplace_back(consumer);
    }
    for (auto& t : cs) t.join();

    bool ok = true;
    for (int i = 0; i < kCount; ++i) {
        ok = ok && processed[i];
    }
    std::cout << (ok ? "OK\n" : "FAIL\n");
    return ok ? 0 : 1;
}
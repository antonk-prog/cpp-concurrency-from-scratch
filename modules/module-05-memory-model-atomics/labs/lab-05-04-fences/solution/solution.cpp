#include <atomic>
#include <cstddef>
#include <iostream>
#include <thread>

constexpr std::size_t kSize = 100;
int data[kSize];
std::atomic<bool> ready{false};
bool verified = false;

void writer() {
    for (std::size_t i = 0; i < kSize; ++i) {
        data[i] = static_cast<int>(i) + 1;
    }
    std::atomic_thread_fence(std::memory_order_release);
    ready.store(true, std::memory_order_relaxed);
}

void reader() {
    while (!ready.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    bool ok = true;
    for (std::size_t i = 0; i < kSize; ++i) {
        if (data[i] != static_cast<int>(i) + 1) {
            ok = false;
            break;
        }
    }
    verified = ok;
}

int main() {
    std::thread w(writer);
    std::thread r(reader);
    r.join();
    w.join();
    if (verified) {
        std::cout << "OK\n";
        return 0;
    }
    std::cout << "FAIL\n";
    return 1;
}
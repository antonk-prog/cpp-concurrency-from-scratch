#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

struct Payload {
    int id;
    std::string text;
};

std::atomic<Payload*> published{nullptr};
constexpr int kExpectedId = 7;
constexpr const char* kExpectedText = "hello";

void producer() {
    Payload* p = new Payload{kExpectedId, kExpectedText};
    published.store(p, std::memory_order_release);
}

void consumer(bool* ok) {
    Payload* p = nullptr;
    while ((p = published.load(std::memory_order_acquire)) == nullptr) {
        std::this_thread::yield();
    }
    *ok = (p->id == kExpectedId && p->text == kExpectedText);
}

int main() {
    constexpr int kConsumers = 4;
    bool results[kConsumers] = {false};

    std::thread prod(producer);
    std::vector<std::thread> consumers;
    for (int i = 0; i < kConsumers; ++i) {
        consumers.emplace_back(consumer, &results[i]);
    }

    prod.join();
    for (auto& t : consumers) t.join();
    delete published.load();

    bool all = true;
    for (bool r : results) {
        all = all && r;
    }
    std::cout << (all ? "OK\n" : "FAIL\n");
    return all ? 0 : 1;
}
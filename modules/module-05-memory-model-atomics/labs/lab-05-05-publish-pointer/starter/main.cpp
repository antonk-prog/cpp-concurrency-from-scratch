#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// ЗАДАНИЕ: реализуй producer() и consumer() с нуля.
//
// producer(): создай Payload, заполни поля, опубликуй указатель через
//             published.store(p, std::memory_order_release).
// consumer(): дождись публикации (load с acquire), проверь поля, запиши
//             результат в *ok.
//
// Не используй memory_order_consume и мьютексы.

struct Payload {
    int id;
    std::string text;
};

std::atomic<Payload*> published{nullptr};
constexpr int kExpectedId = 7;
constexpr const char* kExpectedText = "hello";

void producer() {
    // TODO
}

void consumer(bool* ok) {
    (void)ok;  // TODO
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
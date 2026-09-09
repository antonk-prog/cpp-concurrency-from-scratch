#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <iostream>
#include <algorithm>

// Lock-free стек через CAS.
// TODO: реализуй push() и pop().
//
// Подсказка по структуре:
//   struct node {
//       std::shared_ptr<T> data;
//       node* next;
//       node(T const& d) : data(std::make_shared<T>(d)) {}
//   };
//   std::atomic<node*> head;

template <typename T>
class lock_free_stack {
public:
    void push(T const& data) {
        // TODO: создать узел, подготовить next = head.load(),
        //       опубликовать через CAS-цикл.
        (void)data;
    }

    std::shared_ptr<T> pop() {
        // TODO: CAS-цикл по head, вернуть data извлечённого узла
        //       (или пустой shared_ptr, если стек пуст).
        return std::shared_ptr<T>();
    }
};

int main() {
    const int producers = 4;
    const int consumers = 4;
    const int per_producer = 25000;
    const int total = producers * per_producer;

    lock_free_stack<int> s;
    std::atomic<int> produced(0);

    std::vector<std::thread> pths;
    for (int p = 0; p < producers; ++p) {
        pths.emplace_back([&, p] {
            for (int i = 0; i < per_producer; ++i) {
                s.push(p * per_producer + i);   // уникальные значения
                produced.fetch_add(1);
            }
        });
    }

    std::vector<std::thread> cths;
    std::vector<std::vector<int>> results(consumers);
    for (int c = 0; c < consumers; ++c) {
        cths.emplace_back([&, c] {
            int empty_streak = 0;
            for (;;) {
                auto v = s.pop();
                if (v) {
                    results[c].push_back(*v);
                    empty_streak = 0;
                    continue;
                }
                if (produced.load() == total) break;  // всё извлечено
                if (++empty_streak > total) break;    // предохранитель
                std::this_thread::yield();
            }
        });
    }

    for (auto& th : pths) th.join();
    for (auto& th : cths) th.join();

    // Собираем все извлечённые значения и проверяем, что это ровно {0..total-1}
    std::vector<int> all;
    for (auto& r : results) all.insert(all.end(), r.begin(), r.end());
    std::sort(all.begin(), all.end());

    bool ok = (all.size() == (size_t)total);
    for (int i = 0; ok && i < total; ++i) {
        if (all[i] != i) ok = false;
    }

    std::cout << "extracted = " << all.size() << ", expected = " << total << "\n";
    if (ok) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cout << "FAIL: потери или дубликаты\n";
        return 1;
    }
}
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>

template <typename T>
class threadsafe_queue {
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;

    node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return nullptr;
        }
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue() : head(new node), tail(head.get()) {}
    threadsafe_queue(const threadsafe_queue&) = delete;
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    void push(T new_value) {
        std::shared_ptr<T> new_data =
            std::make_shared<T>(std::move(new_value));
        std::unique_ptr<node> p(new node);
        node* const new_tail = p.get();
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            tail->next = std::move(p);
            tail = new_tail;
        }
    }

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
};

int main() {
    const int total = 100000;
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    threadsafe_queue<int> q;

    std::vector<std::thread> pushers;
    for (int t = 0; t < 4; ++t) {
        pushers.emplace_back([&] {
            for (int i = 0; i < total / 4; ++i) {
                q.push(produced.fetch_add(1));
            }
        });
    }

    std::vector<std::thread> poppers;
    for (int t = 0; t < 4; ++t) {
        poppers.emplace_back([&] {
            for (;;) {
                auto v = q.try_pop();
                if (!v) {
                    if (produced.load() == total) break;
                    continue;
                }
                consumed.fetch_add(1);
            }
        });
    }

    for (auto& th : pushers) th.join();
    for (auto& th : poppers) th.join();

    std::cout << "produced = " << produced.load()
              << ", consumed = " << consumed.load() << "\n";
    if (consumed.load() == total) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cout << "FAIL: потеряны элементы\n";
        return 1;
    }
}
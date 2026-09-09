#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>

template <typename T>
class threadsafe_list {
    struct node {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node() : next() {}
        node(T const& value) : data(std::make_shared<T>(value)) {}
    };

    node head;

public:
    threadsafe_list() {}
    ~threadsafe_list() { remove_if([](T const&) { return true; }); }
    threadsafe_list(threadsafe_list const&) = delete;
    threadsafe_list& operator=(threadsafe_list const&) = delete;

    void push_front(T const& value) {
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head.m);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template <typename Function>
    void for_each(Function f) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            f(*next->data);
            current = next;
            lk = std::move(next_lk);
        }
    }

    template <typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if (p(*next->data)) {
                return next->data;
            }
            current = next;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template <typename Predicate>
    void remove_if(Predicate p) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            if (p(*next->data)) {
                std::unique_ptr<node> old_next = std::move(current->next);
                current->next = std::move(next->next);
                next_lk.unlock();
            } else {
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
};

int main() {
    const int writers = 4;
    const int per_writer = 5000;
    threadsafe_list<int> list;

    std::vector<std::thread> pushers;
    for (int w = 0; w < writers; ++w) {
        pushers.emplace_back([&, w] {
            for (int i = 0; i < per_writer; ++i) {
                list.push_front(w * per_writer + i);
            }
        });
    }
    for (auto& th : pushers) th.join();

    // Конкурентный обход: считаем сумму и число элементов
    std::atomic<long long> sum(0);
    std::atomic<int> count(0);
    std::vector<std::thread> walkers;
    for (int r = 0; r < 4; ++r) {
        walkers.emplace_back([&] {
            list.for_each([&](int v) {
                sum.fetch_add(v);
                count.fetch_add(1);
            });
        });
    }
    for (auto& th : walkers) th.join();

    const long long expected_sum =
        1LL * (writers * per_writer - 1) * (writers * per_writer) / 2;
    const int expected_count = writers * per_writer;

    std::cout << "count = " << count.load()
              << ", expected = " << expected_count << "\n";
    std::cout << "sum = " << sum.load()
              << ", expected = " << expected_sum << "\n";

    // Каждый поток-обходчик видит полный список, значит count кратен 4
    if (count.load() != expected_count * 4 || sum.load() != expected_sum * 4) {
        std::cout << "FAIL: обходы не согласованы\n";
        return 1;
    }

    // find_first_if: ищем существующий и несуществующий элемент
    auto found = list.find_first_if([](int v) { return v == 12345; });
    if (!found || *found != 12345) {
        std::cout << "FAIL: find_first_if не нашёл существующий элемент\n";
        return 1;
    }
    auto missing = list.find_first_if([](int v) { return v == -1; });
    if (missing) {
        std::cout << "FAIL: find_first_if нашёл несуществующий элемент\n";
        return 1;
    }

    // remove_if: удаляем всё, затем обход должен дать пусто
    list.remove_if([](int) { return true; });
    int after = 0;
    list.for_each([&](int) { ++after; });
    if (after != 0) {
        std::cout << "FAIL: remove_if не удалил все элементы (" << after << ")\n";
        return 1;
    }

    std::cout << "OK\n";
    return 0;
}
#include <atomic>
#include <memory>
#include <iostream>

// Lock-free стек с ПЕРЕИСПОЛЬЗОВАНИЕМ узлов (free list).
// Извлечённые узлы возвращаются в пул, новые вставки берут узел из пула.
// Это создаёт риск проблемы ABA — проанализируй код.

template <typename T>
class lock_free_stack {
    struct node {
        T data;
        node* next;
    };
    std::atomic<node*> head{nullptr};
    std::atomic<node*> free_list{nullptr};

    node* allocate() {
        node* n = free_list.load();
        while (n && !free_list.compare_exchange_weak(n, n->next)) {}
        return n ? n : new node;
    }

    void deallocate(node* n) {
        n->next = free_list.load();
        while (!free_list.compare_exchange_weak(n->next, n)) {}
    }

public:
    void push(T const& data) {
        node* n = allocate();           // узел может быть ПЕРЕИСПОЛЬЗОВАН
        n->data = data;
        n->next = head.load();
        while (!head.compare_exchange_weak(n->next, n)) {}
    }

    std::shared_ptr<T> pop() {
        node* old_head = head.load();
        while (old_head &&
               !head.compare_exchange_weak(old_head, old_head->next)) {}
        if (old_head) {
            std::shared_ptr<T> res = std::make_shared<T>(old_head->data);
            deallocate(old_head);       // возвращаем узел в пул!
            return res;
        }
        return std::shared_ptr<T>();
    }
};

int main() {
    lock_free_stack<int> s;
    s.push(1);
    s.push(2);
    auto v = s.pop();
    std::cout << "pop = " << (v ? *v : -1) << "\n";
    return 0;
}
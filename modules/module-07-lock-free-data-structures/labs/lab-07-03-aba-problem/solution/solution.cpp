#include <atomic>
#include <cstdint>
#include <memory>
#include <iostream>

// Lock-free стек с переиспользованием узлов, защищённый от проблемы ABA
// счётчиком версий. Указатель и версия упакованы в одно 64-битное слово:
// версия — в младших 4 битах (узлы выровнены, эти биты свободны).
// CAS выполняется над словом целиком, поэтому «возврат» адреса с другой
// версией провалится. (На других платформах/выравниваниях — своя упаковка.)

template <typename T>
class lock_free_stack {
    struct node {
        T data;
        node* next;
    };
    static constexpr std::uintptr_t VERSION_MASK = 0xF;

    static node* unpack_ptr(std::uintptr_t raw) {
        return reinterpret_cast<node*>(raw & ~VERSION_MASK);
    }
    static unsigned unpack_ver(std::uintptr_t raw) {
        return static_cast<unsigned>(raw & VERSION_MASK);
    }
    static std::uintptr_t pack(node* p, unsigned v) {
        return reinterpret_cast<std::uintptr_t>(p) | (v & VERSION_MASK);
    }

    std::atomic<std::uintptr_t> head{0};
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
        node* n = allocate();
        n->data = data;
        std::uintptr_t old = head.load();
        for (;;) {
            n->next = unpack_ptr(old);
            std::uintptr_t desired = pack(n, unpack_ver(old) + 1);
            if (head.compare_exchange_weak(old, desired)) break;
        }
    }

    std::shared_ptr<T> pop() {
        std::uintptr_t old = head.load();
        for (;;) {
            node* ptr = unpack_ptr(old);
            if (!ptr) return std::shared_ptr<T>();
            std::uintptr_t desired = pack(ptr->next, unpack_ver(old) + 1);
            if (head.compare_exchange_weak(old, desired)) {
                std::shared_ptr<T> res = std::make_shared<T>(ptr->data);
                deallocate(ptr);
                return res;
            }
        }
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
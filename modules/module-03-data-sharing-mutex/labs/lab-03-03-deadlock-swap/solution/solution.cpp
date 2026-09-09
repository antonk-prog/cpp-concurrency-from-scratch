#include <iostream>
#include <mutex>
#include <thread>

struct some_big_object {
    int value;
    explicit some_big_object(int v = 0) : value(v) {}
};

void swap_obj(some_big_object& lhs, some_big_object& rhs) {
    using std::swap;
    swap(lhs.value, rhs.value);
}

class X {
public:
    explicit X(int v) : detail_(v) {}

    int get() const {
        return detail_.value;
    }

    friend void swap(X& lhs, X& rhs) {
        if (&lhs == &rhs) return;
        std::scoped_lock guard(lhs.m_, rhs.m_);
        swap_obj(lhs.detail_, rhs.detail_);
    }

private:
    some_big_object detail_;
    mutable std::mutex m_;
};

int main() {
    X a(1), b(2);
    const int iterations = 1000;

    std::thread t1([&a, &b, iterations] {
        for (int i = 0; i < iterations; ++i) swap(a, b);
    });
    std::thread t2([&a, &b, iterations] {
        for (int i = 0; i < iterations; ++i) swap(b, a);
    });
    t1.join();
    t2.join();

    std::cout << "a=" << a.get() << " b=" << b.get() << "\n";
    return 0;
}
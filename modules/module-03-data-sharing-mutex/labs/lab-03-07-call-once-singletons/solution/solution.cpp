#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class Singleton {
public:
    Singleton() { ++instances; }

    static int instances;
};
int Singleton::instances = 0;

Singleton& get_singleton_static() {
    static Singleton s;   // инициализация один раз и потокобезопасно
    return s;
}

Singleton& get_singleton_call_once() {
    static std::shared_ptr<Singleton> ptr;
    static std::once_flag flag;
    std::call_once(flag, [] { ptr = std::make_shared<Singleton>(); });
    return *ptr;
}

int main() {
    Singleton& s_static = get_singleton_static();
    Singleton* s_once = &get_singleton_call_once();
    std::atomic<bool> ok{true};

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 1000; ++i) {
                if (&get_singleton_static() != &s_static) ok = false;
                if (&get_singleton_call_once() != s_once) ok = false;
            }
        });
    }
    for (auto& t : threads) t.join();

    if (ok.load() && Singleton::instances == 2) {
        std::cout << "All checks passed\n";
    } else {
        std::cout << "Checks failed\n";
        return 1;
    }
    return 0;
}
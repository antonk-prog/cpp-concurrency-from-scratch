#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Account {
    explicit Account(int balance = 0) : balance_(balance) {}
    int balance_ = 0;
    std::mutex m_;
};

void transfer(Account& from, Account& to, int amount) {
    std::lock(from.m_, to.m_);                                // оба разом, без deadlock
    std::lock_guard<std::mutex> g1(from.m_, std::adopt_lock); // владеет уже захваченным
    std::lock_guard<std::mutex> g2(to.m_, std::adopt_lock);   // владеет уже захваченным
    from.balance_ -= amount;
    to.balance_ += amount;
}

int main() {
    Account a{70000};
    Account b{30000};

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&a, &b, t] {
            for (int i = 0; i < 1000; ++i) {
                if ((i + t) % 2 == 0) {
                    transfer(a, b, (i % 20) + 1);
                } else {
                    transfer(b, a, (i % 20) + 1);
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    std::cout << "Total balance: " << a.balance_ + b.balance_ << "\n";
    return 0;
}
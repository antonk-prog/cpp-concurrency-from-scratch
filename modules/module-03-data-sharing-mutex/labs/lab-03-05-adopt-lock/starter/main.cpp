// Лаба 3.5. Допиши transfer: std::lock + lock_guard с adopt_lock.
// Требования — в task.md. Каркас компилируется, но без блокировок итог неверен.
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
    // TODO 1: заблокируй оба мьютекса одной операцией:
    //         std::lock(from.m_, to.m_);

    // TODO 2: оберни уже захваченные мьютексы в lock_guard
    //         с флагом std::adopt_lock (по одной обёртке на мьютекс).

    from.balance_ -= amount;
    to.balance_ += amount;
}

int main() {
    Account a{70000};
    Account b{30000};

    // TODO 3: запусти 4 потока: каждый делает 1000 переводов, чередуя
    //         направление (чётная итерация — a→b, нечётная — b→a),
    //         сумма перевода — (i % 20) + 1. Дождись всех через join().

    std::cout << "Total balance: " << a.balance_ + b.balance_ << "\n";
    return 0;
}
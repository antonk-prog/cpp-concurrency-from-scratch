// Лаба 3.5. Допиши transfer: std::lock + lock_guard с adopt_lock.
// Требования — в task.md. Здесь только Account и transfer.
#include <mutex>

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
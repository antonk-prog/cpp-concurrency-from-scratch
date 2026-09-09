#include <exception>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

struct empty_stack : std::exception {
    const char* what() const noexcept override {
        return "empty stack";
    }
};

template <typename T>
class threadsafe_stack {
public:
    void push(T value) {
        std::lock_guard<std::mutex> guard(m_);
        data_.push(value);
    }

    // ВНИМАНИЕ: этот метод возвращает ССЫЛКУ на защищённый элемент.
    // Мьютекс снимается при выходе из top(), а ссылка остаётся жить —
    // чтение по ней происходит уже БЕЗ блокировки. Плюс интерфейс top()+pop()
    // раздельными вызовами создаёт гонку между чтением и удалением.
    T& top() {
        std::lock_guard<std::mutex> guard(m_);
        if (data_.empty()) throw empty_stack();
        return data_.top();
    }

    void pop() {
        std::lock_guard<std::mutex> guard(m_);
        if (data_.empty()) throw empty_stack();
        data_.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> guard(m_);
        return data_.empty();
    }

private:
    std::stack<T> data_;
    mutable std::mutex m_;
};

// TODO: замени top()+pop() на единый std::shared_ptr<T> pop(),
// который читает и удаляет верхний элемент под одной блокировкой
// (см. лекцию модуля 3, «Минимальные примеры»). В main: заполни стек
// значениями 1..N, запусти несколько потоков, каждый извлекает через
// pop() (ловя empty_stack) и копит сумму; итог должен равняться сумме 1..N.

int main() {
    const int n = 10000;
    threadsafe_stack<int> st;
    for (int i = 1; i <= n; ++i) st.push(i);

    const int threads_count = 4;
    std::vector<int> sums(threads_count, 0);
    std::vector<std::thread> threads;
    for (int t = 0; t < threads_count; ++t) {
        threads.emplace_back([&st, &sums, t] {
            int local = 0;
            for (;;) {
                try {
                    int v = st.top();
                    st.pop();
                    local += v;
                } catch (empty_stack&) {
                    break;
                }
            }
            sums[t] = local;
        });
    }
    for (auto& t : threads) t.join();

    long long total = 0;
    for (int s : sums) total += s;
    long long expected = static_cast<long long>(n) * (n + 1) / 2;
    return total == expected ? 0 : 1;
}
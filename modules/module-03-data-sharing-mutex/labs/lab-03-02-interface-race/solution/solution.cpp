#include <exception>
#include <memory>
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

    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> guard(m_);
        if (data_.empty()) throw empty_stack();
        auto res = std::make_shared<T>(data_.top());
        data_.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> guard(m_);
        return data_.empty();
    }

private:
    std::stack<T> data_;
    mutable std::mutex m_;
};

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
                    auto v = st.pop();
                    local += *v;
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
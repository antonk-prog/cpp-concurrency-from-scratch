#include <iostream>
#include <thread>

void doWork(int value) {
    std::cout << "Worker processed: " << value << "\n";
}

class scoped_thread {
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {}
    scoped_thread(const scoped_thread&) = delete;
    scoped_thread& operator=(const scoped_thread&) = delete;
    ~scoped_thread() {
        if (t.joinable()) {
            t.join();
        }
    }

private:
    std::thread t;
};

std::thread makeWorker(int value) {
    return std::thread(doWork, value);
}

int main() {
    scoped_thread st(makeWorker(7));
    return 0;
}
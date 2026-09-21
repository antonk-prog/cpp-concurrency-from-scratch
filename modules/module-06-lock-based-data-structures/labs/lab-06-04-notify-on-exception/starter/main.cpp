#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <iostream>

// Очередь с условной переменной (см. листинг 6.2 лекции модуля 6).
//
// push() будит ровно одного ожидающего (notify_one()). Если этот разбуженный
// поток бросит исключение, сохраняя элемент, он так и не заберёт элемент из
// очереди. Элемент останется в очереди, но остальные ожидающие потоки
// разбужены не будут — программа может «заснуть» при доступных данных.
//
// TODO: сделай wait_and_pop() безопасной по исключениям. Если присваивание
//   элемента в value бросает:
//     - элемент очереди НЕ должен быть потерян (пусть остаётся в очереди);
//     - разбуди следующего ожидающего (data_cond.notify_one());
//     - пробрось исключение дальше (throw;).
//   Блокировку снимает RAII-обёртка, прямых lock()/unlock() быть не должно.

template <typename T>
class threadsafe_queue {
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue() {}

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        // TODO: обернуть присваивание в try/catch; при исключении
        //       вызвать data_cond.notify_one() и пробросить исключение.
        value = std::move(data_queue.front());
        data_queue.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

// Тип, перемещение-присваивание которого может один раз бросить исключение.
// Так моделируется сбой при сохранении элемента (например, нехватка памяти).
struct payload {
    int value = 0;
    payload() = default;
    explicit payload(int v) : value(v) {}
    payload(const payload&) = delete;
    payload& operator=(const payload&) = delete;
    payload(payload&& other) noexcept : value(other.value) {}
    payload& operator=(payload&& other) {
        if (throw_once.exchange(false)) {
            throw std::runtime_error("simulated failure while storing value");
        }
        value = other.value;
        return *this;
    }
    static inline std::atomic<bool> throw_once{false};
};

int main() {
    threadsafe_queue<payload> q;

    std::atomic<int> ready{0};
    std::atomic<int> succeeded{0};
    std::atomic<int> failed{0};

    std::mutex done_mutex;
    std::condition_variable done_cv;
    int finished = 0;

    auto consumer = [&] {
        ready.fetch_add(1);
        payload out;
        try {
            q.wait_and_pop(out);
            succeeded.fetch_add(1);
        } catch (const std::exception&) {
            failed.fetch_add(1);
        }
        {
            std::lock_guard<std::mutex> lk(done_mutex);
            ++finished;
        }
        done_cv.notify_all();
    };

    std::thread c1(consumer);
    std::thread c2(consumer);

    while (ready.load() != 2) std::this_thread::yield();

    // Демонстрационный sleep (намеренно, синхронизацией не является):
    // даём обоим потребителям дойти до wait_and_pop и заблокироваться.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    payload::throw_once.store(true);
    q.push(payload(7));   // разбудит одного; он бросит при сохранении

    bool stuck;
    {
        std::unique_lock<std::mutex> lk(done_mutex);
        stuck = !done_cv.wait_for(lk, std::chrono::seconds(1),
                                  [&] { return finished == 2; });
    }

    if (stuck) {
        // Разбудим оставшийся поток, чтобы программа не зависла.
        payload::throw_once.store(false);
        q.push(payload(8));
        std::unique_lock<std::mutex> lk(done_mutex);
        done_cv.wait(lk, [&] { return finished == 2; });
    }

    c1.join();
    c2.join();

    std::cout << "succeeded = " << succeeded.load()
              << ", failed = " << failed.load() << "\n";
    if (stuck) {
        std::cout << "BUG: второй ожидающий поток не был разбужен\n";
        return 1;
    }
    std::cout << "OK\n";
    return 0;
}

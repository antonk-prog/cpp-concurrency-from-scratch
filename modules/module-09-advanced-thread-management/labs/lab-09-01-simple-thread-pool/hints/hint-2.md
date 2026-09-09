# Подсказка 2 — механизм, без кода

```text
class thread_pool {
    std::atomic<bool> done;
    thread_safe_queue<std::function<void()>> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            std::function<void()> task;
            if (work_queue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }
public:
    thread_pool() : done(false), joiner(threads) {
        unsigned const count = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < count; ++i)
                threads.emplace_back(&thread_pool::worker_thread, this);
        } catch (...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() { done = true; }

    template <typename FunctionType>
    void submit(FunctionType f) {
        work_queue.push(std::function<void()>(f));
    }
};
```

Порядок полей не случаен: при разрушении сначала `join_threads` присоединит
потоки, потом вектор потоков, и только потом очередь с флагом. Исключение в
конструкторе — `done = true`, чтобы уже запущенные потоки завершились, —
и `throw` для повторной выдачи.
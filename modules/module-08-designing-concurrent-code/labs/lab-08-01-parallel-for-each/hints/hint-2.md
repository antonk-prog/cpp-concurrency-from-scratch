# Подсказка 2 — механизм, без кода

`join_threads` — развитие `thread_guard` на весь вектор:

```text
class join_threads {
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& t) : threads(t) {}
    ~join_threads() {
        for (auto& th : threads)
            if (th.joinable()) th.join();
    }
};
```

Создай `std::vector<std::future<void>> futures(num_threads - 1)` и
`std::vector<std::thread> threads(num_threads - 1)` ДО цикла порождения,
и сразу после них — `join_threads joiner(threads)`. Тогда join произойдёт в
деструкторе при любом исходе.

В цикле:

```text
std::packaged_task<void(void)> task(
    [=] { std::for_each(block_start, block_end, f); });
futures[i] = task.get_future();
threads[i] = std::thread(std::move(task));
```

`std::packaged_task` — move-only, передаётся в поток через `std::move`.

После обработки последнего блока в текущем потоке пройдись по всем
`futures[i].get()`: `get()` не только ждёт завершения, но и повторно выдаёт
исключение, если задача его бросила.
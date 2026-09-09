# Решение 4.2

## parallel_sum

```cpp
long long parallel_sum(const std::vector<int>& data, size_t num_tasks) {
    if (data.empty()) return 0;
    if (num_tasks == 0) num_tasks = 1;

    const size_t block_size = data.size() / num_tasks;
    std::vector<std::future<long long>> futures;

    size_t begin = 0;
    for (size_t t = 0; t < num_tasks; ++t) {
        size_t end = (t == num_tasks - 1) ? data.size() : begin + block_size;
        futures.push_back(std::async(std::launch::async,
            [&data](size_t b, size_t e) {
                return std::accumulate(data.begin() + b, data.begin() + e, 0LL);
            }, begin, end));
        begin = end;
    }

    long long total = 0;
    for (auto& f : futures) {
        total += f.get();
    }
    return total;
}
```

Разбор:

- **`std::launch::async`** — явно требуем отдельный поток для каждой задачи
  (иначе реализация могла бы отложить выполнение до `get()`).
- **Индексы `begin`/`end` передаются аргументами лямбде**, а `data`
  захватывается по ссылке. Каждая задача суммирует свой полуинтервал
  `[b, e)` через `std::accumulate` и возвращает `long long`.
- **Последний блок** идёт до `data.size()` — так учитывается остаток от деления
  (`size % num_tasks` попадает в него).
- **`future.get()`** блокирует поток до готовности фьючерса и возвращает
  результат. Так как задачи запускаются параллельно, `get()`-цикл лишь
  дожидается их — суммирование уже идёт конкурентно.

## Почему `long long`

Сумма 1..1'000'000 = 500'000'500'000, что не влезает в `int` (максимум ~2.1·10⁹).
`std::accumulate` с `0LL` (тип `long long`) и фьючерсы `std::future<long long>`
избегают переполнения.

## Вывод

```
Sequential = 500000500000
Parallel   = 500000500000
Match: yes
```

Совпадение точное: для целочисленного типа порядок сложения не влияет на
результат (в отличие от `float`/`double`). TSan чист: каждый блок читает свой
диапазон, разделяемой записи нет.
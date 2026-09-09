# Решение 2.3

Полная реализация параллельного суммирования. Разбор по шагам.

## 1. Функтор для блока

```cpp
template <typename Iterator, typename T>
struct accumulate_block {
    void operator()(Iterator first, Iterator last, T& result) {
        result = std::accumulate(first, last, result);
    }
};
```

`T&` — чтобы поток писал результат в свою ячейку `results[i]` через `std::ref`.
Именно раздельные ячейки делают запись безопасной без синхронизации: каждый
поток трогает только свою переменную, а `main` читает их после `join()`.

## 2. Число потоков

```cpp
const unsigned long max_threads =
    (length + min_per_thread - 1) / min_per_thread;   // не больше блоков
const unsigned long hardware_threads =
    std::thread::hardware_concurrency();
const unsigned long num_threads =
    std::min(hardware_threads != 0 ? hardware_threads : 2ul, max_threads);
```

Логика: не создавать 32 потока на 5 элементах (огранка сверху по числу блоков)
и не создавать больше потоков, чем аппаратных (иначе переключения контекста
съедят выигрыш). На 0 от `hardware_concurrency()` берём 2. Важно, что
`num_threads` никогда не меньше 1: если `length >= 1`, то `max_threads >= 1`,
а `hardware >= 1` или 2.

## 3. Разбиение на блоки

```cpp
const unsigned long block_size = length / num_threads;
std::vector<T> results(num_threads);
std::vector<std::thread> threads(num_threads - 1);
```

`num_threads - 1` потоков — потому что один «поток» уже есть: сам `main`.
Остаток от деления (`length % num_threads`) попадает в последний блок, который
и считает главный поток.

## 4. Запуск потоков и финальный блок

```cpp
Iterator block_start = first;
for (unsigned long i = 0; i < (num_threads - 1); ++i) {
    Iterator block_end = block_start;
    std::advance(block_end, block_size);
    threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start,
                             block_end, std::ref(results[i]));
    block_start = block_end;
}
accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);
```

Каждый поток получает свой полуинтервал и свою ячейку. `main` считает последний
кусок (включая «хвост») синхронно — так никакая работа не простаивает.

## 5. Сбор результатов

```cpp
for (auto& t : threads) t.join();
return std::accumulate(results.begin(), results.end(), init);
```

`join()` всех потоков — момент синхронизации: после него `results` заполнены
полностью. Остаётся сложить частичные суммы.

## Проверка

```
Sequential: 500000500000
Parallel:   500000500000
Match:      yes
```

(сумма 1..1'000'000 = 500000500000; на машинах с разным числом ядер число
потоков разное, но результат одинаков).

## Важные оговорки

- Для `float`/`double` порядок сложения меняется (блоки), поэтому результат
  `parallel_accumulate` может отличаться от `std::accumulate` — из-за
  неассоциативности операций с плавающей точкой. Для `int` совпадение точное.
- Итераторы должны быть как минимум однонаправленными (`std::distance`,
  `std::advance` их требуют), а `T` — конструируемым по умолчанию (для вектора
  результатов).
- Эта версия не обрабатывает исключения из потоков (это уже глава 8).
# Решение 3.1

## Класс Counter

```cpp
class Counter {
public:
    void increment() {
        std::lock_guard<std::mutex> guard(m_);
        ++value_;
    }
    int get() const {
        std::lock_guard<std::mutex> guard(m_);
        return value_;
    }
private:
    int value_ = 0;
    mutable std::mutex m_;
};
```

Почему так:

- **`std::lock_guard` вместо `lock()`/`unlock()`** — при исключении внутри
  метода RAII-обёртка всё равно разблокирует мьютекс. Прямые вызовы этого
  не гарантируют.
- **`mutable std::mutex`** — чтобы блокировать мьютекс в `get() const`.
  `const` означает «не меняю логическое состояние», но блокировка мьютекса —
  это технический приём, поэтому `mutable` разрешён.
- **Данные и мьютекс вместе** — приватные поля одного класса. Никто снаружи
  не имеет доступа к `value_` без блокировки.

## main

```cpp
const int threads_count = 4;
const int iterations = 100000;

Counter counter;
std::vector<std::thread> threads;
for (int t = 0; t < threads_count; ++t) {
    threads.emplace_back([&counter, iterations] {
        for (int i = 0; i < iterations; ++i) counter.increment();
    });
}
for (auto& t : threads) t.join();
```

Лямбда захватывает `counter` по ссылке — все потоки работают с одним объектом.
`join()` всех потоков гарантирует, что после него счётчик полностью обновлён.

## Почему без мьютекса было бы неверно

`++value_` компилируется в «прочитать → прибавить → записать». Четыре потока
могут прочитать одно и то же значение, прибавить и записать — обновления
теряются. Итог был бы меньше 400000 и менялся бы от запуска к запуску. Мьютекс
делает весь `++value_` неделимым.

## Вывод

```
Final counter: 400000
```

Всегда, на любом числе ядер.
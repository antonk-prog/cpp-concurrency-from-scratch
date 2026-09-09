# Механизм (без кода)

- Класс: `int value_ = 0; mutable std::mutex m_;` (`mutable` — потому что
  `get() const` тоже блокирует мьютекс).
- `increment()`: `std::lock_guard<std::mutex> guard(m_); ++value_;`
- `get() const`: `std::lock_guard<std::mutex> guard(m_); return value_;`
- `main`: 4 потока с лямбдой, каждый цикл `for (int i = 0; i < 100000; ++i)
  counter.increment();` — объект `Counter` захватывается по ссылке (`&counter`).
- После запуска всех потоков — `join()` каждого, затем печать `get()`.
- Не печатай из потоков — общий вывод снова потребует синхронизации.
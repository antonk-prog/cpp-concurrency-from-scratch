# Механизм (без кода)

- Конструктор: `explicit scoped_thread(std::thread t_) : t(std::move(t_)) {}`
  — параметр по значению уже «притянул» владение, в член передаём через
  `std::move`.
- Копирование запрещено: `scoped_thread(const scoped_thread&) = delete;` и
  `operator=(...) = delete;` — иначе два объекта будут звать `join()` одного
  потока.
- Деструктор: `~scoped_thread() { if (t.joinable()) t.join(); }`.
- Возврат: `std::thread makeWorker(int v) { return std::thread(doWork, v); }`
  — временный объект перемещается в caller без лишних копий.
- В `main`: `scoped_thread st(makeWorker(7));` — поток уже в обёртке,
  отдельный `join()` не нужен.
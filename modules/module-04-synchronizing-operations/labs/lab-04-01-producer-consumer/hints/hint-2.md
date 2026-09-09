# Механизм (без кода)

- `push`: `std::lock_guard<std::mutex> lk(mut); data_queue.push(value);`
  затем `data_cond.notify_one();` — уведомление ПОСЛЕ выхода из области
  `lock_guard` (мьютекс свободен, разбуженный потребитель сразу сможет его
  взять).
- `wait_and_pop`: `std::unique_lock<std::mutex> lk(mut);`
  `data_cond.wait(lk, [this] { return !data_queue.empty(); });`
  затем `value = data_queue.front(); data_queue.pop();`
  Предикат обязателен — защита от ложных пробуждений.
- `try_pop`: `std::lock_guard<std::mutex> lk(mut);`
  `if (data_queue.empty()) return false; value = data_queue.front(); data_queue.pop(); return true;`
- В `main` консьюмер в цикле `wait_and_pop(sum)`, пока не обработает N элементов
  (или используй сигнальный элемент, чтобы завершить цикл).
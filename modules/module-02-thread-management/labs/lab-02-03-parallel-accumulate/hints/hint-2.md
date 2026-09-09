# Механизм (без кода)

- Число потоков: `hardware = std::thread::hardware_concurrency();`
  `num_threads = std::min(hardware != 0 ? hardware : 2u, (length + min_per_thread - 1) / min_per_thread);`
  (должен быть как минимум 1).
- Блоки: `block_size = length / num_threads;` «хвост» из остатка достаётся
  последнему блоку, который считает сам `main`.
- `std::vector<T> results(num_threads);` и
  `std::vector<std::thread> threads(num_threads - 1);`
- Запуск: `threads[i] = std::thread(acc_block{}, first_i, last_i, std::ref(results[i]));`
  где `acc_block` — функтор, который зовёт `std::accumulate` и пишет в `T&`.
- После запуска главный поток считает `results[num_threads-1]`, затем
  `for (auto& t : threads) t.join();` и финальный `std::accumulate(results...)`.
- В `main`: вектор 1..N, сравнение с `std::accumulate`.
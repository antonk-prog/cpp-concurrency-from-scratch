# Механизм (без кода)

- Размер блока: `block_size = data.size() / num_tasks;` остаток — в последнем
  блоке: его `end` = `data.end()`.
- Запуск: `std::async(std::launch::async, [&data](size_t b, size_t e) {
    return std::accumulate(data.begin()+b, data.begin()+e, 0LL);
  }, begin, end);` — индексы передаются аргументами, данные захватываются
  по ссылке.
- `std::vector<std::future<long long>> futures;` затем
  `futures.push_back(std::async(...));`
- Сбор: `long long total = 0; for (auto& f : futures) total += f.get();`
- В `main`: вектор 1..1'000'000 через `std::iota`, сравнение с
  `std::accumulate(data.begin(), data.end(), 0LL)`.
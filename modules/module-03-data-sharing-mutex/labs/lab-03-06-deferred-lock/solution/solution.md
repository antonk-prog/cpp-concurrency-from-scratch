# Решение 3.6

## Воркер

```cpp
void worker(long long& shared_total, std::mutex& m) {
    for (int i = 0; i < kIterations; ++i) {
        std::unique_lock<std::mutex> lk(m, std::defer_lock); // объект есть, мьютекс свободен

        long long local = 0;
        for (int j = 0; j < 500; ++j) local += j;   // локальная работа БЕЗ блокировки

        lk.lock();                  // блокировка только на время слияния
        shared_total += local;
        lk.unlock();                // сразу сняли
    }
}
```

Разбор:

- **`std::defer_lock`** — конструктор не блокирует мьютекс, но объект
  `unique_lock` уже связан с ним и готов его взять. Локальная работа идёт
  без блокировки.
- **`lk.lock()` / `lk.unlock()`** — явное управление: мьютекс удерживается
  ровно на участке `shared_total += local;`, а не на всей итерации.
- По завершении итерации `unique_lock` мьютексом не владеет
  (`owns_lock() == false`), поэтому деструктор его не трогает.

## main

```cpp
long long shared_total = 0;
std::mutex m;
std::vector<std::thread> threads;
for (int t = 0; t < kWorkers; ++t) {
    threads.emplace_back([&] { worker(shared_total, m); });
}
for (auto& t : threads) t.join();
```

## Почему итог 998000000

Локальная сумма одной итерации `= 0 + 1 + ... + 499 = 124750`. Всего воркеров 4,
итераций по 2000: `4 * 2000 * 124750 = 998000000`. Гонка потеряла бы часть
обновлений — итог был бы меньше и менялся от запуска к запуску.

## Вывод

```
Shared total: 998000000
```
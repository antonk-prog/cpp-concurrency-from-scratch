# Решение 3.7

## Класс и геттеры

```cpp
class Singleton {
public:
    Singleton() { ++instances; }
    static int instances;
};
int Singleton::instances = 0;

Singleton& get_singleton_static() {
    static Singleton s;   // инициализация один раз и потокобезопасно
    return s;
}

Singleton& get_singleton_call_once() {
    static std::shared_ptr<Singleton> ptr;
    static std::once_flag flag;
    std::call_once(flag, [] { ptr = std::make_shared<Singleton>(); });
    return *ptr;
}
```

Разбор:

- **`static`-локальная переменная** — стандарт гарантирует: инициализация
  выполняется в одном потоке, остальные ждут её завершения. Никакого мьютекса
  вручную.
- **`std::call_once(flag, ...)`** — `flag` помнит, была ли инициализация;
  функция выполняется ровно один раз, а все параллельные вызовы ждут её
  результата. Так лениво инициализируют не только один объект, но и любой
  другой код (открытие файла, заполнение таблицы).
- `instances` увеличивается в конструкторе: по одному экземпляру на синглтон,
  итого `2`. Инкремент происходит в однопоточном контексте (внутри `call_once`
  и при `static`-инициализации), поэтому гонки нет.

## main

```cpp
Singleton& s_static = get_singleton_static();
Singleton* s_once = &get_singleton_call_once();
std::atomic<bool> ok{true};

std::vector<std::thread> threads;
for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&] {
        for (int i = 0; i < 1000; ++i) {
            if (&get_singleton_static() != &s_static) ok = false;
            if (&get_singleton_call_once() != s_once) ok = false;
        }
    });
}
for (auto& t : threads) t.join();
```

- Эталоны фиксируются до запуска потоков; потоки сравнивают с ними адреса.
- `ok` — атомарный: его пишут несколько потоков.
- После `join()` проверка `instances == 2` подтверждает, что каждый синглтон
  создан ровно один раз.

## Вывод

```
All checks passed
```
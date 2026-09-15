# Решение 4.4

```cpp
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

int square(int x) {
    return x * x;
}

int checked_divide(int a, int b) {
    if (b == 0) throw std::runtime_error("division by zero");
    return a / b;
}

int main() {
    std::packaged_task<int(int)> square_task(square);
    std::future<int> square_future = square_task.get_future();
    std::thread squarer(std::move(square_task), 7);
    std::cout << "square(7) = " << square_future.get() << "\n";
    squarer.join();

    std::packaged_task<int(int, int)> divide_task(checked_divide);
    std::future<int> divide_future = divide_task.get_future();
    std::thread divider(std::move(divide_task), 1, 0);
    try {
        int quotient = divide_future.get();
        std::cout << "1 / 0 = " << quotient << "\n";
    } catch (std::exception const& e) {
        std::cout << "caught: " << e.what() << "\n";
    }
    divider.join();

    return 0;
}
```

## Разбор

- **`std::packaged_task<int(int)>`** — сигнатура в шаблоне описывает, что задача
  принимает `int` и возвращает `int`. Сам объект задачи — вызываемый: его
  вызов `task(7)` выполняет обёрнутую функцию и сохраняет результат.
- **`get_future()` вызывается до выполнения.** Фьючерс нужно получить заранее —
  иначе после запуска забрать результат будет нечем. Одна задача — один
  фьючерс.
- **Задача выполняется там, где её вызвали.** Мы передаём её в `std::thread`
  через `std::move` (упакованная задача **move-only**: копировать нельзя) и
  аргумент `7`. Поток вызывает `task(7)`, и после этого связанный фьючерс
  становится готовым.
- **`future.get()`** блокирует вызывающий поток до готовности и возвращает
  значение. Для второй задачи результат — исключение: `get()` **повторно
  выбрасывает** его в вызывающем потоке, поэтому `try/catch` вокруг `get()`
  ловит `runtime_error` изнутри задачи.
- **`join()`** обязателен: потоки должны быть присоединены до завершения
  программы.

Суть приёма: `packaged_task` разрывает «запуск задачи» и «получение
результата». Задачу можно положить в очередь, отдать другому потоку или
пулу, а фьючерс — оставить тому, кому нужен результат.

## Вывод

```
square(7) = 49
caught: division by zero
```

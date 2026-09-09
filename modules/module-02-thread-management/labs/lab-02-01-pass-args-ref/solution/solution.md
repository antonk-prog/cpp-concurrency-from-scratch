# Решение 2.1

Два приёма передачи аргументов в поток.

## Поток 1 — ссылка через `std::ref`

```cpp
void incrementAndSet(int step, int& target) {
    target += step;
}

std::thread t1(incrementAndSet, step, std::ref(counter));
```

Конструктор `std::thread` копирует аргументы и вызывает функцию, передавая
копии как rvalue. Без `std::ref` получилось бы `incrementAndSet(5, <rvalue>)`,
а привязать rvalue к не-const ссылке `int&` нельзя — код бы не собрался.
`std::ref(counter)` создаёт `std::reference_wrapper<int>`, который хранит ссылку
и при вызове разворачивается в настоящий `int&`. Поэтому функция меняет именно
`counter`, а не копию.

## Поток 2 — владение через `std::move`

```cpp
void consume(std::unique_ptr<int> p) {
    if (p) std::cout << "Consumed value: " << *p << "\n";
}

auto p = std::make_unique<int>(99);
std::thread t2(consume, std::move(p));
```

`std::unique_ptr` нельзя копировать, только перемещать. `std::move(p)`
превращает `p` в rvalue, и владение динамическим объектом переходит в хранилище
потока, а затем — в параметр `consume`. После этого `p` в `main` пуст
(`p == nullptr`). Если бы передать `p` напрямую, код не скомпилировался бы
(копирование удалено).

## Вывод

```
Counter after thread 1: 5
Consumed value: 99
Pointer in main is now null: true
```

Порядок детерминирован: `main` печатает «Counter» после `t1.join()`, затем
запускает `t2` (его `consume` печатает «Consumed value») и лишь после
`t2.join()` печатает «Pointer…». Так как `main` ждёт оба потока, все три строки
появляются в ожидаемом порядке.

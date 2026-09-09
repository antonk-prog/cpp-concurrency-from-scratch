# Подсказка 2 — механизм, без кода

В объявлении параллельного поиска:

```text
std::promise<Iterator> result;
bool done_flag = false;              // БЫЛО — гонка
std::atomic<bool> done_flag(false);  // СТАЛО — корректно
```

В операторе вызова `find_element` тип параметра тоже меняется:

```text
std::atomic<bool>* done_flag;
```

Чтение в цикле:

```text
for (; begin != end && !done_flag->load(); ++begin)
```

Установка при нахождении:

```text
result->set_value(begin);
done_flag->store(true);
```

И в `catch`-блоке — тоже `store(true)`. После объединения потоков проверка
«нашлось ли» — это `done_flag.load()`.

Больше ничего менять не нужно: `std::atomic<bool>` по умолчанию использует
`memory_order_seq_cst`, что здесь корректно и достаточно.
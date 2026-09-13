# Решение 3.4

## Класс dns_cache

```cpp
class dns_cache {
public:
    std::string find(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(m_);
        auto it = entries_.find(name);
        return it == entries_.end() ? std::string{} : it->second;
    }

    void update(const std::string& name, const std::string& entry) {
        std::lock_guard<std::shared_mutex> lock(m_);
        entries_[name] = entry;
    }

private:
    std::map<std::string, std::string> entries_;
    mutable std::shared_mutex m_;
};
```

Почему так:

- **`std::shared_lock` в `find`** — общая блокировка: несколько читателей
  входят в `find` одновременно, чтения не сериализуются.
- **`std::lock_guard` в `update`** — монопольная блокировка: пока пишет один
  писатель, не входят ни читатели, ни другие писатели.
- **`mutable`** — блокировка берётся в `const`-методе; мьютекс — техническое
  поле, а не логическое состояние объекта.
- **`return std::string{};`** при отсутствии ключа — отсутствие записи
  обрабатывается значением, а не исключением.

## Проверки в main

Читатель проверяет: если `find` вернул непустую строку, она обязана совпасть
с формулой. Писатель мог ещё не успеть записать ключ — тогда пустая строка
не ошибка.

После `join()` main проверяет все ключи писателей по формулам. При правильной
реализации каждый ключ на месте с точным значением, поэтому выводится
`All checks passed`.

## Почему не обычный std::mutex

С `std::mutex` чтения сериализовались бы: читатели ждали бы друг друга, хотя
не конфликтуют. `std::shared_mutex` пропускает читателей параллельно, а
писателей держит по одному.

## Вывод

```
All checks passed
```
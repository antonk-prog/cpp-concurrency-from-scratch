# Подсказка 2 — механизм, без кода

Изменяющие методы используют `find_entry_for(key)` (он не-`const`, возвращает
`iterator`). Для `const`-метода `value_for` нужен собственный `const`-поиск
(через `std::find_if` по `data`).

**value_for:**

```text
std::shared_lock<std::shared_mutex> lock(mutex);
auto it = std::find_if(data.begin(), data.end(), [&](bucket_value const& item) {
    return item.first == key;
});
return (it == data.end()) ? default_value : it->second;
```

**add_or_update_mapping:**

```text
std::unique_lock<std::shared_mutex> lock(mutex);
auto it = find_entry_for(key);
if (it == data.end()) {
    data.push_back(bucket_value(key, value));
} else {
    it->second = value;
}
```

**remove_mapping:**

```text
std::unique_lock<std::shared_mutex> lock(mutex);
auto it = find_entry_for(key);
if (it != data.end()) {
    data.erase(it);
}
```

Помни: `mutable std::shared_mutex mutex;` — поле помечено `mutable`, потому что
`value_for` — `const`-метод, а блокировка не меняет логическое состояние.
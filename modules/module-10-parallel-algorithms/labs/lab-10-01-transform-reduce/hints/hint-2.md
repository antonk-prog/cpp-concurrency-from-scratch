# Подсказка 2 — механизм, без кода

Объединение карт «карта + карта»:

```text
if (lhs.size() < rhs.size()) std::swap(lhs, rhs);
for (auto const& entry : rhs) lhs[entry.first] += entry.second;
return lhs;
```

Элемент (запись лога) в карту:

```text
++map[log.page]; return map;
```

Две записи лога:

```text
visit_map_type map;
++map[log1.page];
++map[log2.page];
return map;
```

Вызов:

```text
return std::transform_reduce(
    std::execution::par, lines.begin(), lines.end(),
    visit_map_type(), combine_visits(), parse_log_line);
```

`parse_log_line` передаётся как функция преобразования — она вызывается для
каждой строки; её результат (карту или запись) библиотека комбинирует через
`combine_visits` в любом порядке.
# Подсказка 2 — механизм, без кода

Вынеси чтение `tail` в отдельную приватную функцию `get_tail()`, которая
блокирует `tail_mutex`:

```text
node* get_tail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
}
```

В `pop_head()` замени прямое чтение `tail` на вызов `get_tail()`. Обрати внимание:
вызов `get_tail()` должен выполняться **пока удерживается `head_mutex`** (внутри
`pop_head()`) — иначе `head` может «перескочить» за актуальный `tail`
(это объясняется в лекции модуля 6).

Порядок блокировок всегда один: `head_mutex`, затем `tail_mutex` — так deadlock
невозможен.
# Подсказка 2 — механизм, без кода

**push:**

```text
node* new_node = new node(data);
new_node->next = head.load();
while (!head.compare_exchange_weak(new_node->next, new_node)) {
    // при неудаче new_node->next уже обновлён актуальным head
}
```

**pop:**

```text
node* old_head = head.load();
while (old_head &&
       !head.compare_exchange_weak(old_head, old_head->next)) {
    // при неудаче old_head обновлён актуальным head
}
if (old_head) {
    return old_head->data;   // shared_ptr
}
return std::shared_ptr<T>();
```

Почему `compare_exchange_weak`: при неудаче первый аргумент автоматически
получает текущее значение, и цикл просто повторяется — не нужно перезагружать
`head` вручную.

Данные в узле — `std::shared_ptr<T>`: `pop()` возвращает `shared_ptr`, и операция
не может «потерять» значение при исключении (в отличие от возврата по ссылке).
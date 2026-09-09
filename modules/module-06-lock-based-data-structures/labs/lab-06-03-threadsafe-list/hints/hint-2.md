# Подсказка 2 — механизм, без кода

**push_front:**

```text
std::unique_ptr<node> new_node(new node(value));  // вне блокировки
std::lock_guard<std::mutex> lk(head.m);
new_node->next = std::move(head.next);
head.next = std::move(new_node);
```

**for_each / find_first_if — общий каркас:**

```text
node* current = &head;
std::unique_lock<std::mutex> lk(head.m);
while (node* const next = current->next.get()) {
    std::unique_lock<std::mutex> next_lk(next->m);
    lk.unlock();
    ... работа с *next->data ...
    current = next;
    lk = std::move(next_lk);
}
```

**find_first_if:** внутри цикла `if (p(*next->data)) return next->data;`.
По завершении цикла `return std::shared_ptr<T>();`.

**remove_if:**

```text
node* current = &head;
std::unique_lock<std::mutex> lk(head.m);
while (node* const next = current->next.get()) {
    std::unique_lock<std::mutex> next_lk(next->m);
    if (p(*next->data)) {
        std::unique_ptr<node> old_next = std::move(current->next);
        current->next = std::move(next->next);
        next_lk.unlock();            // узел удаляем вне его блокировки
    } else {
        lk.unlock();
        current = next;
        lk = std::move(next_lk);
    }
}
```

Почему безопасно удалять узел после `next_lk.unlock()`: блокировка `current->m`
(через `lk`) всё ещё удерживается, значит ни один поток не сможет захватить
мьютекс удаляемого узла (до него можно добраться только через `current->next`).
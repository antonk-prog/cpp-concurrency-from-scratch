# Подсказка 2 — механизм, без кода

Заполни узел **полностью до публикации**: данные задавай в конструкторе узла,
а не после `head.store`.

```text
node* new_node = new node(data);        // данные создаются до публикации
new_node->next = head.load();
while (!head.compare_exchange_weak(new_node->next, new_node)) {
    // CAS-цикл: узел публикуется только после полной готовности
}
```

То есть структура `node` снова получает конструктор от `T`:

```text
struct node {
    std::shared_ptr<T> data;
    node* next;
    node(T const& d) : data(std::make_shared<T>(d)) {}
};
```

После публикации узел не меняется — его можно только читать.
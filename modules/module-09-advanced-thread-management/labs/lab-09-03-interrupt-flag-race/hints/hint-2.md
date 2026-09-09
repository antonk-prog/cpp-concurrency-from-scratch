# Подсказка 2 — механизм, без кода

Поле становится `std::atomic<bool>`:

```text
class interrupt_flag {
    std::atomic<bool> flag{false};
public:
    void set() { flag.store(true); }
    bool is_set() const { return flag.load(); }
};
```

`std::atomic<bool>` по умолчанию использует `memory_order_seq_cst` — для
кооперативного прерывания этого достаточно: запись `store(true)` станет видима
читающим потокам, а отсутствие гонки уберёт UB.

Больше ничего менять не нужно: схема «флаг + точка прерывания + промис для
доступа к `thread_local` флагу» остаётся прежней. Проверь, что под TSan
решение чистое и все потоки останавливаются.
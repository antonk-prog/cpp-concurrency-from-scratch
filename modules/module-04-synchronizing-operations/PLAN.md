# Модуль 4 — Синхронизация конкурентных операций — ПЛАН

## Допущения модуля

### Прогрессия (что разрешено)
Всё из главы 4 (C++17 std):
- `std::condition_variable`: `wait(lock, pred)` / `wait(lock)`, `notify_one()`,
  `notify_all()`, `wait_for()`/`wait_until()` (с предикатом и без).
- `std::future` / `std::shared_future`: `get()`, `wait()`, `wait_for()`/`wait_until()`,
  `valid()`, `share()`.
- `std::async` с `std::launch::async` / `std::launch::deferred` (и `|`).
- `std::promise`: `get_future()`, `set_value()`, `set_exception()`,
  `set_value_at_thread_exit()`; `broken_promise`.
- `std::packaged_task`: `get_future()`, вызов через `operator()`.
- `<chrono>`: `duration` + литералы (`_ms` и др.), `time_point`, `system_clock`,
  `steady_clock`, `high_resolution_clock`, `duration_cast`, `now()`,
  `std::this_thread::sleep_for()/sleep_until()`.
- Timed-блокировки: `std::timed_mutex`, `std::recursive_timed_mutex`,
  `std::shared_timed_mutex` — `try_lock_for()/try_lock_until()`,
  `try_lock_shared_for()/try_lock_shared_until()`; `unique_lock`/`shared_lock`
  с `duration`/`time_point`.
- Потокобезопасная очередь (листинг 4.5) как мост из главы 3 (мьютексы +
  условная переменная).

### Concurrency TS (лекционно, по книге)
Глава 4 рассматривает и Concurrency TS (в курсе материал книги, см.
ESTABLISHED A002): продолжения `std::experimental::future::then()`, `when_all`,
`when_any`, `latch`, `barrier`, `flex_barrier`, FP-стиль и CSP/акторы. Эти темы
**обязательно описываются в лекции** (с пояснениями и листингами по A014),
но **в лабах не используются** — примитивы `std::experimental` не гарантированно
доступны в компиляторах; лабы строятся на стандартных примитивах C++17, которые
собираются везде.

Запрещено:
- Всё из C++20+ (по ESTABLISHED A002): `std::jthread`, `std::barrier`,
  `std::latch` и т.д.
- Атомики/ordering (гл. 5), структуры данных (гл. 6–7), пулы (гл. 9).
- Синхронизация через `sleep_for` (стиль курса): `sleep` — только демо с
  комментарием, не как механизм синхронизации.

### Последствие для лаб
Лабы строятся на: producer-consumer через условную переменную; `std::async` +
`std::future` для параллельных вычислений; `std::promise`/`std::packaged_task`
для передачи результата/исключения между потоками. «Найди и почини» — реальный
баг, ловимый TSan (например, гонка за общий буфер без мьютекса, или
`notify_one` до `wait` без предиката — последний TSan не ловит, поэтому баг
выбираем так, чтобы ловился).

### Список лаб (тип, название, суть, чему учит)

1. **lab-04-01-producer-consumer (допиши TODO, средняя)**
   - Суть: класс `threadsafe_queue` с `std::condition_variable`; продюсер кладёт
     числа в очередь и `notify_one()`, консьюмер ждёт через `wait_and_pop()`.
     Дописать `wait()`-логику и `notify`.
   - Чему учит: `condition_variable` + `unique_lock`; почему `wait` требует
     предикат (spurious wakeup); зачем `unique_lock` (wait снимает/забирает
     мьютекс).

2. **lab-04-02-async-parallel-sum (напиши с нуля, базовая)**
   - Суть: разбить суммирование диапазона на N блоков, каждый через
     `std::async`, собрать результаты через `future.get()`; сравнить с
     однопоточным `std::accumulate`.
   - Чему учит: `std::async`, `std::future`, `get()` как блокирующее ожидание;
     декомпозиция на независимые задачи.

3. **lab-04-03-promise-exception (найди и почини, средняя/сложная)**
   - Суть: стартовый код передаёт результат через `std::promise`; в одном из
     путей `promise` уничтожается без `set_value` → `broken_promise` у
     ожидающего; починить (гарантировать установку значения/исключения).
   - Чему учит: `std::promise`/`std::future`, `set_value`/`set_exception`,
     `broken_promise`, распространение исключений через фьючерсы.

> Варианты типов: допиши TODO, напиши с нуля, найди и почини — чередуются,
> не повторяются.

### Объём
- Лекция ~4000–6000+ слов (по OPEN A012: увеличивать минимум в 2–3 раза),
  с вставленными листингами (4.1/4.5 — очередь, 4.6 — async, 4.11 — wait_until):
  слово «листинг» — ссылка на код, код подписан; рисунки из книги — места
  вставки помечены с названием и ссылаемостью (A016).
- При написании — отчёт об объёме пользователю (A015).
- 3 лабы: средняя, базовая, средняя/сложная.

## Заголовки лекции (README.md) с 1–2 фразами содержания

1. **Зачем это нужно** — почему «опрос флага + sleep» плох: трата CPU,
   неверный интервал; зачем ждать именно событие (условная переменная) или
   единичный результат (фьючерс).

2. **Как это работает** — условная переменная: `wait`/`notify_one`/`notify_all`,
   роль `unique_lock` и предиката, spurious wakeup; фьючерсы: `async`,
   `future`/`shared_future`, `promise`, `packaged_task`; исключения через
   фьючерсы и `broken_promise`; ожидание с таймаутом: `chrono`, `_for`/`_until`;
   поды подхода к синхронизации из книги: FP-стиль (фьючерсы как данные),
   передача сообщений/CSP (акторы), продолжения и `when_all`/`when_any`/
   `latch`/`barrier` из Concurrency TS (лекционно).

3. **Минимальные примеры** — [листинг 4.1/4.5] потокобезопасная очередь с
   `wait_and_pop`; [листинг 4.6] `std::async` + `get()`; promise/future-пара;
   [листинг 4.11] `wait_until` с таймаутом.

4. **Типичные ошибки** — `wait` без предиката и гонка notify/wait (потерянное
   уведомление); забыл `notify_one`; использовал `lock_guard` вместо
   `unique_lock` в `wait`; `broken_promise`; `get()` дважды; сон как механизм
   синхронизации.

5. **Шпаргалка** — таблица: `condition_variable` (wait/notify/wait_for),
   `future` (get/wait/wait_for), `async` (launch), `promise`, `packaged_task`,
   `chrono` (duration/time_point/clock), timed-мьютексы; отдельно — краткий
   список тем Concurrency TS (then, when_all, when_any, latch, barrier) со
   ссылкой на разделы книги.

## Дополнительно
- Лаборатории модуля — таблица в README (ссылка, тип, сложность, суть). Без ответов.
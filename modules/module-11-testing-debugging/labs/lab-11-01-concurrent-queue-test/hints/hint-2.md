# Подсказка 2 — механизм, без кода

Тело `push`-задачи:

```text
push_ready.set_value();
ready.wait();
q.push(42);
```

Тело `pop`-задачи:

```text
pop_ready.set_value();
ready.wait();
return q.pop();
```

Финальные проверки после `push_done.get()`:

```text
ok = ok && (pop_done.get() == 42);
ok = ok && q.empty();
```

`q.empty()` проверяется после завершения обоих фьючерсов — очередь должна
стать пустой. В `catch (...)` не забудь `go.set_value(); throw;`, иначе
упавший поток оставит других ждать сигнала навсегда.
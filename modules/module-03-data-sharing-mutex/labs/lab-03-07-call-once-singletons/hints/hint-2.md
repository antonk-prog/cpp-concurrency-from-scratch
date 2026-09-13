# Механизм (без кода)

- Вариант 1: внутри геттера — `static std::shared_ptr<Singleton> ptr;`
  и `static std::once_flag flag;`; затем `std::call_once(flag, [] { ptr = ...; });`
  и `return *ptr;`.
- Вариант 2: внутри геттера — `static Singleton s; return s;`. Инициализация
  `static`-локальной переменной в C++11+ выполняется один раз и потокобезопасно.
- `int Singleton::instances` увеличивается в конструкторе; оба синглтона создают
  по одному экземпляру → `instances == 2`.
- Общий флаг проверок в `main` сделай `std::atomic<bool>`, чтобы не создать
  гонку при записи из нескольких потоков.
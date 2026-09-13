# Модуль 9. Усовершенствованное управление потоками

## Лекция

### Зачем это нужно

В предыдущих модулях мы управляли потоками вручную: создавали их, следили
за `join`, делили данные на блоки. Но запуск потока — дорогая операция, а
потоки — ограниченный ресурс. Создавать новый поток на каждую задачу —
расточительство: на маленьких задачах издержки создания съедают весь выигрыш,
а на больших параллельных задачах число потоков начинает неограниченно расти.

**Пул потоков** (thread pool) решает обе проблемы: заранее создаётся
фиксированное число рабочих потоков, задачи складываются в очередь, и рабочие
потоки по одной их выполняют. Потоки создаются один раз, а не на каждую
задачу.

Насколько дорого создавать поток? На типичном настольном процессоре запуск
`std::thread` — это системный вызов, выделение стека и настройка структур
планировщика: порядок десятков микросекунд. Для задачи, которая выполняется
миллисекунду, это терпимо; для задачи в микросекунды — накладные расходы в
десятки процентов. Пул окупается, когда потоков порождается много и они живут
недолго: один раз заплатили за потоки, дальше только очередь и выполнение.

Вторая половина главы — **прерывание потоков**. Не всегда поток должен
дожидаться естественного завершения: пользователь отменил операцию, приложение
завершается, задача стала неактуальной. Потоку нужно отправить сигнал
«остановись», чтобы он аккуратно завершил работу — освободил ресурсы и вышел.
В C++17 стандартного механизма прерывания нет, но его несложно построить
самому. Этим и займёмся.

### Простейший пул потоков

Простейший пул: фиксированное число рабочих потоков (обычно по числу
аппаратных потоков, `std::thread::hardware_concurrency()`), очередь задач и
функция `submit()`, которая кладёт задачу в очередь. Каждый рабочий поток
крутится в цикле: берёт задачу из очереди, выполняет, берёт следующую. В
самом простом варианте возможность дождаться завершения задачи не
предусмотрена — если она нужна, синхронизацией занимаемся сами.

**Листинг 9.1. Простой пул потоков**

```cpp
class thread_pool {
    std::atomic<bool> done;
    thread_safe_queue<std::function<void()>> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            std::function<void()> task;
            if (work_queue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }

public:
    thread_pool() : done(false), joiner(threads) {
        unsigned const count = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < count; ++i) {
                threads.emplace_back(&thread_pool::worker_thread, this);
            }
        } catch (...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() {
        done = true;
    }

    template <typename FunctionType>
    void submit(FunctionType f) {
        work_queue.push(std::function<void()>(f));
    }
};
```

Разбор ключевых деталей.

- **Очередь.** Используем потокобезопасную очередь из модуля 6
  (`push`/`try_pop`). Очередь задач разделяется всеми потоками.
- **Задачи.** Функция `submit()` упаковывает переданный вызываемый объект в
  `std::function<void()>` и кладёт в очередь. `std::function` требует, чтобы
  сохранённый объект был копируемым — для простых функций это так.
- **Число потоков.** Берём `hardware_concurrency()`. Это значение — подсказка:
  код не учитывает потоки других частей приложения и других процессов, о чём
  говорилось в модуле 8.
- **Исключения в конструкторе.** Запуск потока может провалиться. В `catch`
  устанавливаем флаг `done`, чтобы уже запущенные потоки завершились, и
  выбрасываем исключение заново.
- **Порядок объявления полей.** Флаг `done` и очередь объявлены ДО вектора
  потоков, а вектор — до `joiner`. При уничтожении поля разрушаются в обратном
  порядке: сначала `joiner` присоединит все потоки, потом разрушится вектор
  потоков, и только потом очередь и флаг. Нельзя удалить очередь, пока её
  используют рабочие потоки.
- **Цикл рабочего потока.** `while (!done)`: берёт задачу, выполняет; если
  задач нет — `std::this_thread::yield()`, уступает процессорное время, чтобы
  другие потоки могли добавить работу.

Такого пула достаточно для независимых задач, не возвращающих значений и не
выполняющих блокирующих операций. Но если задача должна вернуть результат —
приходится синхронизироваться вручную. А в некоторых ситуациях простой пул
вообще приводит к взаимной блокировке. Улучшим его.

### Ожидание завершения задач

В модуле 4 мы ждали результаты через `std::future` и `std::async`. С пулом
то же самое можно сделать, перенеся синхронизацию внутрь пула: `submit()`
будет возвращать `std::future`, и вызывающий код сможет ждать завершения
задачи и получать результат.

Для этого нужны две вещи.

**Первая — `std::packaged_task`.** Оборачиваем переданную функцию в
`packaged_task<result_type()>`: задача сохранит результат (или исключение) во
фьючерсе, который можно получить через `get_future()`.

**Вторая — move-only обёртка задачи.** `std::packaged_task` не допускает
копирования — только перемещение. А `std::function` требует копируемости
сохранённого объекта. Значит, в очереди нельзя хранить `std::function<void()>`.
Нужна обёртка, умеющая хранить move-only объекты:

**Листинг 9.2. `function_wrapper` — move-only обёртка задачи**

```cpp
class function_wrapper {
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() {}
    };
    std::unique_ptr<impl_base> impl;

    template <typename F>
    struct impl_type : impl_base {
        F f;
        impl_type(F&& f_) : f(std::move(f_)) {}
        void call() { f(); }
    };

public:
    template <typename F>
    function_wrapper(F&& f) : impl(new impl_type<F>(std::move(f))) {}

    void call() { impl->call(); }

    function_wrapper(function_wrapper&& other) : impl(std::move(other.impl)) {}
    function_wrapper& operator=(function_wrapper&& other) {
        impl = std::move(other.impl);
        return *this;
    }

    function_wrapper(function_wrapper const&) = delete;
    function_wrapper& operator=(function_wrapper const&) = delete;
};
```

Идея: шаблонный конструктор сохраняет любой вызываемый объект (включая
move-only) в конкретном типе `impl_type<F>`, а интерфейс предоставляет
виртуальный `call()`. Обёртка хранит `std::unique_ptr<impl_base>` и потому
сама только перемещается. Для нашей задачи функций без параметров,
возвращающих `void`, этого достаточно.

Теперь `submit()` возвращает `std::future`:

**Листинг 9.3. `submit()`, возвращающий `std::future`**

```cpp
class thread_pool {
    std::deque<function_wrapper> work_queue;

public:
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f) {
        typedef typename std::result_of<FunctionType()>::type result_type;

        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res(task.get_future());
        work_queue.push_back(std::move(task));
        return res;
    }
};
```

`std::result_of<FunctionType()>::type` — тип, возвращаемый вызовом `f()`. Это
стандартный способ «узнать» тип результата функции до её вызова. Фьючерс
получаем из `packaged_task` до помещения задачи в очередь; задача кладётся в
очередь через `std::move`, потому что `packaged_task` move-only. Всё
остальное в пуле не меняется — рабочему потоку всё равно, что хранится в
очереди, он лишь вызывает `task.call()`.

С таким пулом перепишем `parallel_accumulate` из модуля 2. Отличие от
ручной версии: работа делится не на число потоков, а на число блоков
(`num_blocks`), а пул сам раздаёт блоки рабочим потокам.

**Листинг 9.4. `parallel_accumulate` на пуле потоков**

```cpp
template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    unsigned long const length = std::distance(first, last);
    if (!length) return init;

    unsigned long const block_size = 25;
    unsigned long const num_blocks = (length + block_size - 1) / block_size;

    std::vector<std::future<T>> futures(num_blocks - 1);
    thread_pool pool;

    Iterator block_start = first;
    for (unsigned long i = 0; i < num_blocks - 1; ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        futures[i] = pool.submit(
            [block_start, block_end]() {
                return std::accumulate(block_start, block_end, T());
            });
        block_start = block_end;
    }
    T last_result = std::accumulate(block_start, last, T());

    T result = init;
    for (unsigned long i = 0; i < num_blocks - 1; ++i)
        result += futures[i].get();
    return result + last_result;
}
```

Блоков должно быть заметно больше, чем потоков, чтобы пул мог эффективно
распределять работу. Но слишком мелкие блоки — тоже плохо: передача задачи
пулу, запуск в рабочем потоке и возврат результата через `future` стоят
денег. Если блок слишком мелкий, пул будет работать медленнее одного потока.
Оптимальный размер подбирается под задачу.

Здесь важно почувствовать разницу с ручным порождением потоков из модуля 2.
Там число потоков было фиксированным и совпадало с числом блоков: каждый
поток обрабатывал ровно свой диапазон. С пулом потоков работа дробится на
мелкие куски, и пул сам раздаёт их свободным рабочим потокам. Если ядро
простаивает — оно возьмёт следующий блок; если нет — блок подождёт в очереди.
Это даёт более равномерную загрузку, но цена — накладные расходы на каждую
задачу и на каждый `future`.

Исключения тоже обрабатываются: любое исключение из задачи сохраняется
`packaged_task` и повторно выдаётся из `get()`. Если же исключение покидает
`parallel_accumulate` до `get()` — деструктор пула установит `done` и
присоединит все потоки, так что «утечки» потоков не будет.

Когда пул, а когда `std::async`? Для независимых задач `std::async` проще:
библиотека сама решает, сколько потоков запускать, и сама ждёт их при
уничтожении фьючерсов. Пул окупается, когда нужно точное управление числом
потоков (например, нельзя допускать переоценку вычислительных возможностей),
задачи короткие и их много, или когда задачи должны «помогать» друг другу в
ожидании — то, что `std::async` не умеет. Правило простое: простые случаи —
`std::async`, сложные сценарии повторного использования потоков — пул.

Пул с ожиданием результата отлично работает для независимых задач. Но
представьте задачу, которая сама отправляет в пул другую задачу и ждёт её
результата. Вот здесь и начинаются проблемы.

### Задачи, ожидающие завершения других задач

Возьмём Quicksort. Разбиваем данные на две части и сортируем обе рекурсивно.
Чтобы использовать доступную конкурентность, меньшую часть отдаём пулу и ждём
её результат через `get()`. Если бы мы запускали поток на каждый вызов —
число потоков росло бы лавинообразно. Но с пулом возникает другая беда:
число потоков ограничено, и если ВСЕ рабочие потоки окажутся в состоянии
ожидания результата задачи, которая ещё не была запланирована (потому что не
нашлось свободного потока), — взаимная блокировка. Все ждут, а работать некому.

Заметим: `std::async` из модуля 4 этой проблемы не имеет. Библиотека сама
решает, запустить задачу в новом потоке или выполнить её синхронно при вызове
`get()`. Если потоков больше не осталось, асинхронный вызов просто выполнится
в том потоке, который его запустил, — «задача ждёт задачу» не возникает. Пул
же с фиксированным числом потоков такой роскоши лишён: рабочий поток не может
«превратиться» в исполнителя своей же задачи, он обязан ждать, пока не
освободится поток. Отсюда и необходимость выполнять чужие задачи во время
ожидания.

В главе 8 мы решали похожую проблему, выполняя работу вместо ожидания: поток
брал фрагмент из стека и сортировал его, пока ждал нужный результат. С пулом
то же самое можно автоматизировать: дать потоку возможность выполнять задачи
из очереди, пока он ждёт результата своей задачи.

Добавим в пул функцию `run_pending_task()` — одну итерацию из цикла рабочего
потока:

**Листинг 9.5. `run_pending_task()`**

```cpp
void thread_pool::run_pending_task() {
    function_wrapper task;
    if (work_queue.try_pop(task)) {
        task();
    } else {
        std::this_thread::yield();
    }
}
```

А рабочий поток теперь просто вызывает эту функцию в цикле. Разница между
`worker_thread()` и `run_pending_task()` в том, что вторую может вызывать любой
поток — например, тот, который ждёт результат своей задачи.

**Листинг 9.6. Quicksort на пуле с `run_pending_task()`**

```cpp
template <typename T>
struct sorter {
    thread_pool pool;

    std::list<T> do_sort(std::list<T>& chunk_data) {
        if (chunk_data.empty()) return chunk_data;

        std::list<T> result;
        result.splice(result.begin(), chunk_data, chunk_data.begin());
        T const& partition_val = *result.begin();

        auto divide_point = std::partition(
            chunk_data.begin(), chunk_data.end(),
            [&](T const& val) { return val < partition_val; });

        std::list<T> new_lower_chunk;
        new_lower_chunk.splice(
            new_lower_chunk.end(), chunk_data,
            chunk_data.begin(), divide_point);

        std::future<std::list<T>> new_lower = pool.submit(
            [this, chunk = std::move(new_lower_chunk)]() mutable {
                return do_sort(chunk);
            });

        std::list<T> new_higher(do_sort(chunk_data));
        result.splice(result.end(), new_higher);
        while (new_lower.wait_for(std::chrono::seconds(0)) !=
               std::future_status::ready) {
            pool.run_pending_task();
        }
        result.splice(result.begin(), new_lower.get());
        return result;
    }
};

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
    if (input.empty()) return input;
    sorter<T> s;
    return s.do_sort(input);
}
```

Ключевой момент — цикл `while (... wait_for(0) != ready) run_pending_task();`.
Вместо пустого ожидания поток выполняет чужие задачи из очереди. Это не просто
«занять бездельника»: это гарантия отсутствия взаимной блокировки. Пока
ожидающий поток делает полезную работу, система продвигается, и рано или
поздно нужный результат появится.

Теперь у пула есть `submit()` и `run_pending_task()`, оба обращаются к одной
очереди. Из модуля 8 мы знаем, что общий изменяемый набор данных — источник
конкуренции. Займёмся этим.

### Снижение конкуренции за очередь работ

Каждый вызов `submit()` кладёт элемент в общую очередь, каждый рабочий поток
постоянно её опустошает. При росте числа процессоров растёт конкуренция за
очередь — даже свободная от блокировок очередь заставляет ядра перебрасывать
строки кэша.

Простой способ уменьшить конкуренцию — дать каждому потоку собственную
очередь. Новые задачи кладутся в очередь текущего потока, и только если своей
очереди нет (поток не из пула) — в общую. Для этого пригодится
`thread_local` — переменная, у которой у каждого потока своя копия:

**Листинг 9.7. Пул с локальными очередями работ**

```cpp
class thread_pool {
    thread_safe_queue<function_wrapper> pool_work_queue;
    typedef std::queue<function_wrapper> local_queue_type;
    static thread_local std::unique_ptr<local_queue_type> local_work_queue;

    void worker_thread() {
        local_work_queue.reset(new local_queue_type);
        while (!done) {
            run_pending_task();
        }
    }

public:
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f) {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(f);
        std::future<result_type> res(task.get_future());
        if (local_work_queue) {
            local_work_queue->push(std::move(task));
        } else {
            pool_work_queue.push(std::move(task));
        }
        return res;
    }

    void run_pending_task() {
        function_wrapper task;
        if (local_work_queue && !local_work_queue->empty()) {
            task = std::move(local_work_queue->front());
            local_work_queue->pop();
            task();
        } else if (pool_work_queue.try_pop(task)) {
            task();
        } else {
            std::this_thread::yield();
        }
    }
};
```

Почему `std::unique_ptr<local_queue_type>` в `thread_local`: нам не нужна
собственная очередь у потоков, не входящих в пул, — например, у основного
потока, который отправляет задачи через `submit()`. Указатель инициализируется
в `worker_thread()` до цикла, а деструктор `unique_ptr` при выходе из потока
уничтожит очередь. `submit()` проверяет `local_work_queue`: если указатель
есть — поток из пула, кладём в локальную очередь; иначе — в общую.
`run_pending_task()` сначала обслуживает локальную очередь, потом общую.
Локальная очередь — обычный `std::queue`, ведь к ней обращается один поток.

Проблема такого подхода — **неравномерность**. Если у одного потока скопилось
много работы, а у других пусто, они простаивают, хотя работы много. Пример —
Quicksort: первый блок попадёт в общую очередь, но все порождённые блоки
окажутся в локальной очереди того потока, который его обработал. Другие
потоки будут простаивать. Решение — **перехват работы**.

### Перехват работы

Пусть безработный поток может взять работу из очереди другого потока. Для
этого каждый поток должен зарегистрировать свою очередь в пуле. Очередь,
поддерживающая перехват, должна позволять владельцу класть и забирать задачи с
одного конца, а чужим потокам — «красть» с другого. Свободная от блокировок
реализация сложна; мы, как и в книге, обойдёмся мьютексом — перехват редкое
событие, и конкуренция за мьютекс невелика:

**Листинг 9.8. Очередь для перехвата работы**

```cpp
class work_stealing_queue {
    typedef function_wrapper data_type;
    std::deque<data_type> the_queue;
    mutable std::mutex the_mutex;

public:
    work_stealing_queue() {}
    work_stealing_queue(work_stealing_queue const&) = delete;
    work_stealing_queue& operator=(work_stealing_queue const&) = delete;

    void push(data_type data) {
        std::lock_guard<std::mutex> lock(the_mutex);
        the_queue.push_front(std::move(data));
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop(data_type& res) {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (the_queue.empty()) return false;
        res = std::move(the_queue.front());
        the_queue.pop_front();
        return true;
    }

    bool try_steal(data_type& res) {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (the_queue.empty()) return false;
        res = std::move(the_queue.back());
        the_queue.pop_back();
        return true;
    }
};
```

`push()` и `try_pop()` работают с началом очереди, `try_steal()` — с концом.
Для владельца очередь ведёт себя как стек (LIFO): последняя добавленная задача
выполняется первой. Это полезно для кэша (данные последней задачи скорее всего
ещё в кэше) и хорошо согласуется с Quicksort: глубокая, только что созданная
задача выполняется раньше мелких веток, уменьшая число активных задач. А
чужой поток крадёт с другого конца — это снижает конкуренцию между `try_pop`
и `try_steal`.

Отдельно отметим: `try_steal` и `try_pop` защищены одним мьютексом, поэтому
владелец и вор всё равно конкурируют за строку кэша, на которой лежит мьютекс.
Полностью убрать эту конкуренцию позволяет свободная от блокировок очередь с
атомарными указателями на оба конца — это тема главы 7, и в рамках данного
модуля мы ограничиваемся блокирующей версией, потому что перехват — событие
редкое.

Теперь пул с перехватом: пул создаёт очереди сам, хранит их в векторе и
передаёт каждому потоку индекс его очереди:

**Листинг 9.9. Пул с перехватом работы**

```cpp
class thread_pool {
    typedef function_wrapper task_type;

    std::atomic<bool> done;
    thread_safe_queue<task_type> pool_work_queue;
    std::vector<std::unique_ptr<work_stealing_queue>> queues;
    std::vector<std::thread> threads;
    join_threads joiner;

    static thread_local work_stealing_queue* local_work_queue;
    static thread_local unsigned my_index;

    void worker_thread(unsigned my_index_) {
        my_index = my_index_;
        local_work_queue = queues[my_index].get();
        while (!done) {
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_type& task) {
        return local_work_queue && local_work_queue->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type& task) {
        return pool_work_queue.try_pop(task);
    }

    bool pop_task_from_other_thread_queue(task_type& task) {
        for (unsigned i = 0; i < queues.size(); ++i) {
            unsigned const index = (my_index + i + 1) % queues.size();
            if (queues[index]->try_steal(task)) {
                return true;
            }
        }
        return false;
    }

public:
    thread_pool() : joiner(threads), done(false) {
        unsigned const thread_count = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < thread_count; ++i) {
                queues.push_back(std::unique_ptr<work_stealing_queue>(
                    new work_stealing_queue));
                threads.emplace_back(&thread_pool::worker_thread, this, i);
            }
        } catch (...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() { done = true; }

    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f) {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(f);
        std::future<result_type> res(task.get_future());
        if (local_work_queue) {
            local_work_queue->push(std::move(task));
        } else {
            pool_work_queue.push(std::move(task));
        }
        return res;
    }

    void run_pending_task() {
        task_type task;
        if (pop_task_from_local_queue(task) ||
            pop_task_from_pool_queue(task) ||
            pop_task_from_other_thread_queue(task)) {
            task();
        } else {
            std::this_thread::yield();
        }
    }
};
```

Теперь в `thread_local` хранится указатель на очередь, созданную пулом
(память под неё выделяет конструктор пула), и индекс потока. `run_pending_task()`
пробует три источника в порядке: свою очередь → общую очередь → чужие очереди.
Перехват обходит очереди остальных потоков, начиная со следующей за своей
(смещение `(my_index + i + 1) % size`), чтобы все потоки не кидались на первую
очередь. Если бы каждый безработный поток начинал обход с очереди потока 0,
тот превратился бы в узкое место: все воры атаковали бы одну очередь.

Это рабочий пул для множества задач. Дальше его можно развивать — например,
динамически менять размер под нагрузку, когда потоки блокируются на
вводе-выводе. Это оставим за рамками курса.

### Прерывание потоков

Вторая тема главы — как остановить долгоиграющий поток. Причины бывают
разные: приложение завершается, пользователь отменил операцию, задача стала
неактуальной. Во всех случаях суть одна: один поток должен попросить другой
остановиться ДО естественного завершения его работы. И сделать это нужно так,
чтобы поток мог завершиться аккуратно — освободить ресурсы, привести данные в
порядок, — а не быть убитым принудительно.

В C++11-17 стандартного механизма прерывания нет. Но создать его несложно.
Прерывание **кооперативное**: поток, который хотят остановить, сам решает,
в каких местах он готов проверить запрос на прерывание. В этих местах он
вызовет «точку прерывания».

#### Интерфейс и инициализация флага

Снаружи нужен класс, похожий на `std::thread`, с дополнительной функцией
`interrupt()`:

```cpp
class interruptible_thread {
    std::thread internal_thread;
    interrupt_flag* flag;

public:
    template <typename FunctionType>
    interruptible_thread(FunctionType f) {
        std::promise<interrupt_flag*> p;
        internal_thread = std::thread([f, &p] {
            p.set_value(&this_thread_interrupt_flag);
            f();
        });
        flag = p.get_future().get();
    }

    void join() { internal_thread.join(); }
    void detach() { internal_thread.detach(); }
    bool joinable() const { return internal_thread.joinable(); }

    void interrupt() {
        if (flag) {
            flag->set();
        }
    }
};
```

Флаг прерывания — `thread_local` переменная `this_thread_interrupt_flag`: у
каждого потока своя копия, поэтому `interruption_point()` вызывается без
параметров — она читает флаг текущего потока. Почему нельзя обойтись только
`std::thread`? Потому что `interrupt()` вызывается из ДРУГОГО потока и должен
достучаться до флага прерываемого потока. Для этого при запуске в новый поток
передаётся адрес его флага через `std::promise`/`std::future`:
`thread_local`-объект создаётся при первом обращении, лямбда сохраняет его
адрес в промис, а `interruptible_thread` дожидается фьючерса и запоминает
указатель. К моменту возврата из конструктора ссылка на локальный промис `p`
в лямбде уже не используется — поток либо досчитал до `set_value`, либо мы
ждём в `get()`.

Почему именно «рукопожатие» через промис, а не просто передача адреса в
конструкторе? Потому что `thread_local`-объект создаётся не в момент запуска
потока, а при первом обращении к нему из этого потока. Пока новый поток не
начал выполняться, его флага ещё физически нет. Промис даёт гарантию: к
моменту, когда конструктор вернёт управление, флаг уже создан, и `interrupt()`
можно вызывать безопасно.

`thread_local` — это «переменная класса хранения»: память выделяется
отдельно для каждого потока, и при завершении потока объект уничтожается.
Именно на этом свойстве держится вся схема: у каждого потока собственный
флаг, и `interruption_point()` всегда знает, где искать.

Важный нюанс про время жизни. Указатель `flag` в `interruptible_thread`
указывает на `thread_local`-объект прерываемого потока. Если поток завершился
сам, а мы всё ещё вызываем `interrupt()` по этому объекту — это висячий
указатель. Поэтому `interrupt()` нужно вызывать, пока поток жив, а после
`detach()` полагаться на него нельзя вовсе: отсоединённый поток может
завершиться в любой момент, и флаг будет уничтожен. Реализация из книги это
честно отдаёт на откуп программисту: очистку флага при выходе из потока и при
отсоединении нужно продумать самому.

#### Обнаружение прерывания

Самый простой способ проверить запрос — функция `interruption_point()`:
если флаг установлен, она бросает исключение `thread_interrupted`:

```cpp
void interruption_point() {
    if (this_thread_interrupt_flag.is_set()) {
        throw thread_interrupted();
    }
}
```

Вызываем её в коде там, где прерывание безопасно:

```cpp
void foo() {
    while (!done) {
        interruption_point();
        process_next_item();
    }
}
```

Но это не покрывает главный случай: поток может быть заблокирован в ожидании —
на условной переменной, на фьючерсе. Пока он спит, он не может вызвать
`interruption_point()`. Нужно уметь прерывать само ожидание.

#### Прерывание ожидания на `std::condition_variable`

Главные места, где поток «спит», — ожидание уведомления от условной
переменной, готовности фьючерса или освобождения мьютекса. Чтобы поток можно
было прервать там, само ожидание должно уметь пробуждаться по сигналу
прерывания. Начнём с условной переменной.

Идея проста: при установке флага разбудить ожидающий поток уведомлением
условной переменной, а сразу после пробуждения — проверить флаг. Для этого в
`interrupt_flag` хранится указатель на условную переменную, которую нужно
уведомить в `set()`.

**Листинг 9.10. `interrupt_flag` и `interruptible_wait` с таймаутом**

```cpp
class interrupt_flag {
    std::atomic<bool> flag;
    std::condition_variable* thread_cond;
    std::mutex set_clear_mutex;

public:
    interrupt_flag() : thread_cond(nullptr) {}

    void set() {
        flag.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        if (thread_cond) {
            thread_cond->notify_all();
        }
    }

    bool is_set() const {
        return flag.load(std::memory_order_relaxed);
    }

    void set_condition_variable(std::condition_variable& cv) {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = &cv;
    }

    void clear_condition_variable() {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = nullptr;
    }

    struct clear_cv_on_destruct {
        ~clear_cv_on_destruct() {
            this_thread_interrupt_flag.clear_condition_variable();
        }
    };
};

void interruptible_wait(std::condition_variable& cv,
                        std::unique_lock<std::mutex>& lk) {
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    interruption_point();
    cv.wait_for(lk, std::chrono::milliseconds(1));
    interruption_point();
}
```

Почему здесь нельзя просто `cv.wait(lk)`? Две проблемы.

**Первая — безопасность исключений.** `wait()` может бросить исключение, и
связь «флаг → условная переменная» останется, а поток уже вышел из ожидания.
Следующий `interrupt()` уведомит мёртвую переменную. Лечится RAII-стражей
`clear_cv_on_destruct`, снимающим связь в деструкторе.

**Вторая — гонка.** Между проверкой `interruption_point()` и вызовом `wait()`
может прийти `interrupt()`: флаг уже установлен, уведомление отправлено, но
поток ещё не в состоянии ожидания — и уведомление пропадёт. Поток уснёт
навсегда. Решение из книги — `wait_for` с таймаутом 1 мс: поток просыпается
как минимум раз в миллисекунду, проверяет флаг и выходит. Установка флага
дополнительно будит потоки через `notify_all` — для того и нужна связь с
условной переменной.

Цена — частые ложные пробуждения. Зато решается вопрос для всех ожидающих:
они и так обязаны обрабатывать ложные пробуждения, так что уведомление от
прерывания выглядит как ещё одно ложное пробуждение. Именно поэтому в `set()`
используется `notify_all()`, а не `notify_one()`: мы не знаем, какой поток
ждёт на этой условной переменной, и не можем рисковать тем, что уведомление
уйдёт «не тому» — пусть проснутся все, лишние проверят флаг и продолжат ждать.

Если есть предикат, таймаут прячется внутрь цикла:

```cpp
template <typename Predicate>
void interruptible_wait(std::condition_variable& cv,
                        std::unique_lock<std::mutex>& lk,
                        Predicate pred) {
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    while (!this_thread_interrupt_flag.is_set() && !pred()) {
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    interruption_point();
}
```

#### Прерывание ожидания на `std::condition_variable_any`

`std::condition_variable_any` умеет работать с любым типом блокировки. Это
открывает более чистое решение: мы можем перехватить блокировку и снять её
внутри ожидания — ровно в тот момент, когда поток находится в состоянии
ожидания.

Идея: собственный тип блокировки `custom_lock`. При создании он блокирует
внутренний мьютекс `set_clear_mutex` и записывает адрес `condition_variable_any`
в флаг. `condition_variable_any::wait()` вызывает `unlock()` нашего объекта —
тот снимает и внешнюю блокировку, и внутренний мьютекс. Пока поток спит,
прерывающий поток может заблокировать `set_clear_mutex`, проверить связь и
уведомить условную переменную. Когда ожидание завершается, `wait()` вызывает
`lock()` — снова берём оба мьютекса, — и мы проверяем флаг до снятия связи.

**Листинг 9.11. Прерываемое ожидание на `condition_variable_any`**

```cpp
class interrupt_flag {
    std::atomic<bool> flag;
    std::condition_variable* thread_cond;
    std::condition_variable_any* thread_cond_any;
    std::mutex set_clear_mutex;

public:
    interrupt_flag() : thread_cond(nullptr), thread_cond_any(nullptr) {}

    void set() {
        flag.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        if (thread_cond) {
            thread_cond->notify_all();
        } else if (thread_cond_any) {
            thread_cond_any->notify_all();
        }
    }

    template <typename Lockable>
    void wait(std::condition_variable_any& cv, Lockable& lk) {
        struct custom_lock {
            interrupt_flag* self;
            Lockable& lk;

            custom_lock(interrupt_flag* self_,
                        std::condition_variable_any& cond,
                        Lockable& lk_)
                : self(self_), lk(lk_) {
                self->set_clear_mutex.lock();
                self->thread_cond_any = &cond;
            }

            void unlock() {
                lk.unlock();
                self->set_clear_mutex.unlock();
            }

            void lock() {
                std::lock(self->set_clear_mutex, lk);
            }

            ~custom_lock() {
                self->thread_cond_any = nullptr;
                self->set_clear_mutex.unlock();
            }
        };

        custom_lock cl(this, cv, lk);
        interruption_point();
        cv.wait(cl);
        interruption_point();
    }
};

template <typename Lockable>
void interruptible_wait(std::condition_variable_any& cv, Lockable& lk) {
    this_thread_interrupt_flag.wait(cv, lk);
}
```

Здесь нет таймаутов и нет частых пробуждений. `unlock()` вызывается только
когда поток уже внутри `wait()` — значит, прерывающий поток гарантированно
разбудит уснувшего, а не «в никуда». Это то, чего мы не могли добиться с
`std::condition_variable`: у неё нет способа вклиниться между проверкой флага
и вызовом `wait()`.

#### Прерывание других блокирующих вызовов

С мьютексами и фьючерсами так красиво не выйдет: нет простого способа прервать
само ожидание. Приходится снова использовать таймауты — в цикле.

**Листинг 9.12. Прерываемое ожидание `std::future`**

```cpp
template <typename T>
void interruptible_wait(std::future<T>& uf) {
    while (!this_thread_interrupt_flag.is_set()) {
        if (uf.wait_for(std::chrono::milliseconds(1)) ==
            std::future_status::ready) {
            break;
        }
    }
    interruption_point();
}
```

Ожидание длится до установки флага или готовности фьючерса, при каждом проходе
цикла блокируясь максимум на 1 мс. В среднем прерывание распознаётся через
полмиллисекунды; если системные часы крупнее — дольше. Таймаут можно уменьшить,
но тогда поток будет чаще просыпаться и тратить время на переключения. Это
компромисс между отзывчивостью на прерывание и ценой частых пробуждений:
мелкий таймаут — быстрее реакция, но больше холостых переключений контекста.
Тот же приём применим и к мьютексам (через `try_lock` в цикле), и к любым
другим блокирующим вызовам, которые не дают себя прервать изнутри.

#### Обработка прерываний

С точки зрения прерываемого потока прерывание — это исключение
`thread_interrupted`. Его можно перехватить и обработать как любое другое:

```cpp
try {
    do_something();
} catch (thread_interrupted&) {
    handle_interruption();
}
```

Если перехватить и продолжить, следующая точка прерывания (если `interrupt()`
вызовут ещё раз) бросит исключение снова. Это удобно для цикла независимых
задач: прерывание текущей задачи отменяет её, и поток переходит к следующей.

Поскольку прерывание — исключение, в коде, который может быть прерван, должны
действовать обычные правила безопасности исключений: RAII-обёртки освободят
ресурсы, инварианты не нарушатся.

Если позволить `thread_interrupted` распространиться за пределы функции,
переданной конструктору `std::thread`, будет вызван `std::terminate` — программа
упадёт. Чтобы этого не случилось, обёртываем вызов пользовательской функции в
`catch (thread_interrupted const&) {}` прямо в конструкторе `interruptible_thread`:

```cpp
internal_thread = std::thread([f, &p] {
    p.set_value(&this_thread_interrupt_flag);
    try {
        f();
    } catch (thread_interrupted const&) {}
});
```

Тогда необработанное прерывание завершит только этот поток, а не всю программу.

#### Прерывание фоновых задач при выходе из приложения

Типичный сценарий — приложение, которое в фоне отслеживает изменения файловой
системы и обновляет индекс поиска. Фоновый поток живёт весь жизненный цикл
приложения. При завершении его нужно аккуратно закрыть.

**Листинг 9.13. Фоновые задачи и их прерывание**

```cpp
std::mutex config_mutex;
std::vector<interruptible_thread> background_threads;

void background_thread(int disk_id) {
    while (true) {
        interruption_point();
        fs_change fsc = get_fs_changes(disk_id);
        if (fsc.has_changes()) {
            update_index(fsc);
        }
    }
}

void start_background_processing() {
    background_threads.emplace_back(background_thread, disk_1);
    background_threads.emplace_back(background_thread, disk_2);
}

int main() {
    start_background_processing();
    process_gui_until_exit();

    std::unique_lock<std::mutex> lk(config_mutex);
    for (auto& t : background_threads) {
        t.interrupt();
    }
    for (auto& t : background_threads) {
        t.join();
    }
}
```

Почему сначала прерываем ВСЕ потоки, а потом ждём каждый? Если прерывать и
сразу join каждый поток по очереди, прерывающий поток будет ждать завершения
первого, хотя мог бы ещё прервать остальных. А потоки завершаются не сразу:
им нужно дойти до следующей точки прерывания, выполнить деструкторы. Прервав
всех сразу, мы позволяем им обрабатывать прерывания параллельно, и общее время
завершения сокращается. Входить в ожидание стоит лишь тогда, когда нечем
заняться, — то есть когда все уже прерваны.

Это общий принцип конкурентного завершения: сначала раздать всем сигналы об
остановке, дать им завершаться параллельно, и только потом собирать результаты
через `join`. Последовательная схема «прервать одного → дождаться → прервать
следующего» превращает параллельную обработку в цепочку последовательных
ожиданий.

### Типичные ошибки

**Порядок объявления полей пула.** Очередь и флаг должны быть объявлены до
вектора потоков, а вектор — до `join_threads`. Иначе при уничтожении очередь
разрушится раньше, чем потоки закончат её использовать.

**`std::function` вместо move-only обёртки.** `std::packaged_task` нельзя
копировать, а `std::function` хранит только копируемые объекты. Для задач с
`packaged_task` нужен `function_wrapper`.

**Задача ждёт задачу.** Если в пуле с ограниченным числом потоков задача
блокируется в `get()` на результате другой задачи, а свободных потоков нет, —
взаимная блокировка. Лечится `run_pending_task()` во время ожидания.

**`get()` без обработки ожидания.** Ожидая результат, поток обязан помогать
пулу выполнять чужие задачи — иначе при полной загрузке пула он «застрянет».

**Неатомарный флаг прерывания.** Флаг читается прерываемым потоком, а пишется
прерывающим — только `std::atomic<bool>` (подробнее в лабе 9.3).

**Проверка флага «в никуда».** При `cv.wait()` прерывание может прийти между
проверкой флага и входом в ожидание — уведомление пропадёт, поток уснёт
навсегда. Нужен `wait_for` с таймаутом или `condition_variable_any` с
`custom_lock`.

**Забыл снять связь с условной переменной.** Если не очистить указатель на
cv в RAII-стражем, `interrupt()` уведомит мёртвую переменную. Всегда страж.

**Прерывание без обработки исключения.** Распространение `thread_interrupted`
за пределы функции потока → `std::terminate`. Перехватывать в конструкторе.

**join по одному потоку сразу после прерывания.** Сначала прервать всех,
потом ждать каждого — иначе тратим время на ожидание, когда ещё есть работа.

**Один рабочий поток и «задача ждёт задачу».** В пуле с одним потоком любая
задача, ждущая результат другой задачи, — гарантированный deadlock: рабочий
занят, очередь некому обслуживать. Даже в пуле с несколькими потоками хватит
пары вложенных ожиданий, чтобы заблокировать все.

**Хранение move-only задачи в `std::function`.** `std::function` требует
копируемости; `packaged_task` — нет. Пока задачи без результата — `std::function`
подходит; как только понадобился `future` — только `function_wrapper`.

**Слишком мелкие блоки в `submit()`.** На передачу задачи и возврат результата
через `future` уходят ресурсы. При очень мелких блоках пул проигрывает
однопоточному коду. Подбирай размер блока под задачу, измеряй.

### Шпаргалка

| Приём | Где применяется | Что даёт |
|-------|-----------------|----------|
| Пул потоков | повторяющиеся задачи | потоки создаются один раз, задачи — в очереди |
| `submit()` + `future` | возврат результата из задачи | ожидание завершения и результат без ручной синхронизации |
| `function_wrapper` | move-only задачи | хранение `packaged_task` в очереди |
| `run_pending_task()` | ожидание результата задачи | выполнение чужих задач вместо пустого ожидания, защита от deadlock |
| `thread_local` очередь | снижение конкуренции за очередь | у каждого потока своя очередь |
| Перехват работы | неравномерное распределение | безработный поток крадёт работу у занятого |
| `try_steal` с конца | очередь с перехватом | владелец работает с начала (LIFO, кэш), чужой — с конца |
| `interrupt_flag` (`thread_local`) | прерывание потоков | сигнал «остановись» конкретному потоку |
| `interruption_point()` | безопасные места | проверка запроса, бросок `thread_interrupted` |
| `wait_for` + `notify_all` | прерываемое ожидание на cv | пробуждение из сна, проверка флага |
| `custom_lock` для `cv_any` | прерываемое ожидание на `condition_variable_any` | снятие блокировки внутри `wait()`, прерывание без таймаутов |
| `interrupt()` всех → потом join | завершение фоновых задач | параллельная обработка прерываний |

## Лаборатории модуля

| Лаба | Тип | Суть одной строкой |
|------|-----|--------------------|
| [9.1. Простой пул потоков](labs/lab-09-01-simple-thread-pool/task.md) | напиши с нуля | реализуй пул потоков: очередь задач, `submit`, флаг `done`, join всех |
| [9.2. Задача ждёт задачу](labs/lab-09-02-task-waits-task/task.md) | предскажи вывод | объясни, почему зависает пул, когда задача ждёт результата другой задачи |
| [9.3. Гонка за флаг прерывания](labs/lab-09-03-interrupt-flag-race/task.md) | найди и почини | почини неатомарный флаг в `interruptible_thread` |
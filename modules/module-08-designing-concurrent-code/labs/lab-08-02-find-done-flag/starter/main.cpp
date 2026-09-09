#include <algorithm>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

// БАГ: общий флаг остановки — обычный bool.
// Он читается всеми потоками (в цикле) и пишется тем, кто нашёл элемент.
// Чтение/запись неатомарной переменной из разных потоков без
// синхронизации — гонка за данные (UB).
//
// TODO: исправь — сделай флаг std::atomic<bool> и используй load()/store().

template <typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match) {
    struct find_element {
        void operator()(Iterator begin, Iterator end, MatchType match,
                        std::promise<Iterator>* result, bool* done_flag) {
            try {
                for (; begin != end && !*done_flag; ++begin) {
                    if (*begin == match) {
                        result->set_value(begin);
                        *done_flag = true;
                        return;
                    }
                }
            } catch (...) {
                try {
                    result->set_exception(std::current_exception());
                    *done_flag = true;
                } catch (...) {}
            }
        }
    };

    unsigned long const length = std::distance(first, last);
    if (!length) return last;

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads =
        (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware = std::thread::hardware_concurrency();
    unsigned long const num_threads =
        std::min(hardware ? hardware : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::promise<Iterator> result;
    bool done_flag = false;
    std::vector<std::thread> threads(num_threads - 1);
    {
        // join_threads joiner(threads);  // (RAII, как в лекции)

        Iterator block_start = first;
        for (unsigned long i = 0; i < num_threads - 1; ++i) {
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            threads[i] = std::thread(find_element(),
                                     block_start, block_end, match,
                                     &result, &done_flag);
            block_start = block_end;
        }
        find_element()(block_start, last, match, &result, &done_flag);
    }
    for (auto& th : threads)
        if (th.joinable()) th.join();

    if (!done_flag) return last;
    return result.get_future().get();
}

int main() {
    const int n = 200000;
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);

    const int match = 0;   // лежит в самом начале — потоки перекрываются
    const int repeats = 300;

    auto found = parallel_find(data.begin(), data.end(), match);
    for (int r = 1; r < repeats; ++r) {
        found = parallel_find(data.begin(), data.end(), match);
    }

    bool ok = (found != data.end() && *found == match);
    std::cout << "found = " << *found
              << " at " << std::distance(data.begin(), found) << "\n";
    std::cout << "result = " << (ok ? "OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
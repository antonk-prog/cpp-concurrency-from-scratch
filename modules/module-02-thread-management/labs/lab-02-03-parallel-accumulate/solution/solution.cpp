#include <algorithm>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

template <typename Iterator, typename T>
struct accumulate_block {
    void operator()(Iterator first, Iterator last, T& result) {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    const unsigned long length = std::distance(first, last);
    if (length == 0) {
        return init;
    }

    const unsigned long min_per_thread = 25;
    const unsigned long max_threads =
        (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads =
        std::thread::hardware_concurrency();
    const unsigned long num_threads =
        std::min(hardware_threads != 0 ? hardware_threads : 2ul, max_threads);

    const unsigned long block_size = length / num_threads;
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);

    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(),
            block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);

    for (auto& t : threads) {
        t.join();
    }
    return std::accumulate(results.begin(), results.end(), init);
}

int main() {
    const int n = 1000000;
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 1);

    const long long sequential =
        std::accumulate(data.begin(), data.end(), 0LL);
    const long long parallel =
        parallel_accumulate(data.begin(), data.end(), 0LL);

    std::cout << "Sequential: " << sequential << "\n";
    std::cout << "Parallel:   " << parallel << "\n";
    std::cout << "Match:      " << (sequential == parallel ? "yes" : "no") << "\n";
    return 0;
}
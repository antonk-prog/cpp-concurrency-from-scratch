#include <future>
#include <iostream>
#include <numeric>
#include <vector>

long long parallel_sum(const std::vector<int>& data, size_t num_tasks) {
    if (data.empty()) return 0;
    if (num_tasks == 0) num_tasks = 1;

    const size_t block_size = data.size() / num_tasks;
    std::vector<std::future<long long>> futures;

    size_t begin = 0;
    for (size_t t = 0; t < num_tasks; ++t) {
        size_t end = (t == num_tasks - 1) ? data.size() : begin + block_size;
        futures.push_back(std::async(std::launch::async,
            [&data](size_t b, size_t e) {
                return std::accumulate(data.begin() + b, data.begin() + e, 0LL);
            }, begin, end));
        begin = end;
    }

    long long total = 0;
    for (auto& f : futures) {
        total += f.get();
    }
    return total;
}

int main() {
    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 1);

    const long long sequential =
        std::accumulate(data.begin(), data.end(), 0LL);
    const long long parallel = parallel_sum(data, 4);

    std::cout << "Sequential = " << sequential << "\n";
    std::cout << "Parallel   = " << parallel << "\n";
    std::cout << "Match: " << (sequential == parallel ? "yes" : "no") << "\n";
    return 0;
}
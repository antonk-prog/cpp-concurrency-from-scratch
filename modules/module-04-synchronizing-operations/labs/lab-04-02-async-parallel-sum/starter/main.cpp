// Лаба 4.2. Напиши parallel_sum с нуля.
// Требования — в task.md. Каркас, который компилируется.
#include <numeric>
#include <vector>

long long parallel_sum(const std::vector<int>& data, size_t num_tasks) {
    // Напиши: разбиение на блоки, std::async для каждого блока,
    // сбор результатов через future.get().
    (void)data;
    (void)num_tasks;
    return 0;
}

int main() {
    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 1);

    // Напиши: parallel_sum(data, 4) и сравнение с однопоточным accumulate.
    return 0;
}
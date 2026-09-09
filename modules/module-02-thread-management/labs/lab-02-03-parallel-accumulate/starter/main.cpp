// Лаба 2.3. Напиши parallel_accumulate с нуля.
// Требования — в task.md. Здесь каркас, который компилируется.
#include <algorithm>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

int main() {
    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 1);

    // Напиши parallel_accumulate и сверь результат с однопоточным accumulate.
    return 0;
}
#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>

// Проверка: заполнен ли вектор последовательностью 1..N.
bool is_ascending(std::vector<int> const& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] != static_cast<int>(i) + 1) return false;
    }
    return true;
}

int main() {
    const int n = 1000;

    // Политика seq: один поток, без перемежения, но порядок НЕ специфицирован.
    std::vector<int> v(n);
    int count = 0;
    std::for_each(std::execution::seq, v.begin(), v.end(),
                  [&](int& x) { x = ++count; });

    bool ok = is_ascending(v);
    std::cout << "seq: первые: " << v[0] << " " << v[1] << " " << v[2]
              << "... последние: " << v[n - 3] << " " << v[n - 2] << " " << v[n - 1] << "\n";
    std::cout << "последовательность 1..N с seq: " << (ok ? "да" : "НЕТ") << "\n";
    std::cout << "гарантировано ли это стандартом: НЕТ\n";
    return 0;
}
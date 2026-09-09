#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>

bool is_ascending(std::vector<int> const& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] != static_cast<int>(i) + 1) return false;
    }
    return true;
}

int main() {
    const int n = 1000;

    // Политика seq: порядок НЕ гарантирован.
    std::vector<int> v_seq(n);
    int count_seq = 0;
    std::for_each(std::execution::seq, v_seq.begin(), v_seq.end(),
                  [&](int& x) { x = ++count_seq; });

    // Обычный for_each без политики: порядок гарантирован.
    std::vector<int> v_plain(n);
    int count_plain = 0;
    std::for_each(v_plain.begin(), v_plain.end(),
                  [&](int& x) { x = ++count_plain; });

    bool seq_ok = is_ascending(v_seq);
    bool plain_ok = is_ascending(v_plain);

    std::cout << "seq:   последовательность 1..N: "
              << (seq_ok ? "да" : "НЕТ") << "\n";
    std::cout << "plain: последовательность 1..N: "
              << (plain_ok ? "да" : "НЕТ") << "\n";
    std::cout << "seq гарантирует порядок: НЕТ (политика отменяет гарантию)\n";
    std::cout << "plain гарантирует порядок: ДА\n";
    return 0;
}
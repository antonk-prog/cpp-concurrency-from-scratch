#include <execution>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

struct log_info {
    std::string page;
    std::string browser;
};

// Дано: разбор строки лога вида "page,browser".
log_info parse_log_line(std::string const& line) {
    auto comma = line.find(',');
    log_info info;
    info.page = line.substr(0, comma);
    info.browser = line.substr(comma + 1);
    return info;
}

using visit_map_type = std::unordered_map<std::string, unsigned long long>;

// TODO: реализуй комбинирующий функтор (четыре перегрузки operator()).
struct combine_visits {
    // TODO: visit_map_type operator()(visit_map_type lhs, visit_map_type rhs) const
    //       — свести по меньшей карте: if (lhs.size() < rhs.size()) swap; обойти rhs.
    // TODO: visit_map_type operator()(log_info log, visit_map_type map) const
    // TODO: visit_map_type operator()(visit_map_type map, log_info log) const
    // TODO: visit_map_type operator()(log_info log1, log_info log2) const
};

// TODO: реализуй функцию через std::transform_reduce(par, ...).
visit_map_type count_visits_per_page(std::vector<std::string> const& lines) {
    (void)lines;  // TODO: убрать после реализации
    return visit_map_type();
}

int main() {
    const int n = 200000;
    const char* pages[] = {"/index.html", "/about", "/docs", "/blog", "/shop"};
    const char* browsers[] = {"Chrome", "Firefox", "Safari"};

    std::vector<std::string> lines;
    lines.reserve(n);
    for (int i = 0; i < n; ++i) {
        lines.push_back(std::string(pages[i % 5]) + "," + browsers[i % 3]);
    }

    visit_map_type result = count_visits_per_page(lines);

    // Эталонный последовательный подсчёт.
    visit_map_type expected;
    for (auto const& line : lines) {
        ++expected[parse_log_line(line).page];
    }

    bool ok = (result == expected);
    std::cout << "pages = " << result.size() << ", expected = " << expected.size() << "\n";
    std::cout << "result = " << (ok ? "OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
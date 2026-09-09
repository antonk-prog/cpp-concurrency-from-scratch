#include <algorithm>
#include <execution>
#include <iostream>
#include <string>
#include <vector>

class my_exception {};

void run(bool with_policy) {
    std::vector<int> v(10);

    try {
        if (with_policy) {
            std::for_each(std::execution::seq, v.begin(), v.end(),
                          [](int) { throw my_exception(); });
        } else {
            std::for_each(v.begin(), v.end(),
                          [](int) { throw my_exception(); });
        }
        std::cout << "no exception\n";
    } catch (my_exception const&) {
        std::cout << "CAUGHT\n";
    }
}

int main(int argc, char** argv) {
    bool with_policy = (argc > 1 && std::string(argv[1]) == "--policy");
    std::cout << "mode: " << (with_policy ? "seq-политика" : "без политики") << "\n";
    run(with_policy);

    std::cout << "итог: без политики исключение перехватывается (CAUGHT);\n";
    std::cout << "      с политикой std::execution::seq исключение приводит\n";
    std::cout << "      к std::terminate и не доходит до catch\n";
    return 0;
}
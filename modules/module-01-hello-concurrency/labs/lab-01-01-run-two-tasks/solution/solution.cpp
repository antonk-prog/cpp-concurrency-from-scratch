#include <iostream>
#include <thread>

void computeSum(int n, int& sum) {
    sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += i;
    }
}

void countMultiplesOfSeven(int n, int& count) {
    count = 0;
    for (int i = 1; i <= n; ++i) {
        if (i % 7 == 0) {
            ++count;
        }
    }
}

int main() {
    const int n = 100;
    int sum = 0;
    int count = 0;

    std::thread sumThread(computeSum, n, std::ref(sum));
    std::thread countThread(countMultiplesOfSeven, n, std::ref(count));

    sumThread.join();
    countThread.join();

    std::cout << "Sum 1.." << n << " = " << sum << "\n";
    std::cout << "Multiples of 7 in 1.." << n << " = " << count << "\n";
    return 0;
}

#include <cstddef>
#include "poet/core/dynamic_for.hpp"

template<std::size_t Unroll = 8>
int dynamic_for_sum(std::size_t begin, std::size_t end) {
    int total = 0;
    poet::dynamic_for<Unroll>(begin, end, [&](std::size_t index){ total += static_cast<int>(index); });
    return total;
}

int manual_sum(std::size_t begin, std::size_t end) {
    int total = 0;
    for (std::size_t i = begin; i < end; ++i) total += static_cast<int>(i);
    return total;
}

// Simple microbenchmark comparing `dynamic_for_sum` and `manual_sum`.
// Measures wall-clock time for repeated calls and prints results.
#include <chrono>
#include <iostream>

int main() {
    constexpr std::size_t begin = 0;
    constexpr std::size_t end = 4096;
    constexpr std::size_t iterations = 20000; // total iterations of the whole sum

    // Warm-up
    volatile int w = dynamic_for_sum(begin, end) + manual_sum(begin, end);
    (void)w;

    // dynamic_for (unroll 8) benchmark
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        long long acc = 0;
        for (std::size_t i = 0; i < iterations; ++i) acc += dynamic_for_sum<8>(begin, end);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dt = t1 - t0;
        std::cout << "dynamic_for_sum<8>: " << dt.count() << " s, result: " << acc << "\n";
    }

    // dynamic_for (unroll 4) benchmark
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        long long acc = 0;
        for (std::size_t i = 0; i < iterations; ++i) acc += dynamic_for_sum<4>(begin, end);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dt = t1 - t0;
        std::cout << "dynamic_for_sum<4>: " << dt.count() << " s, result: " << acc << "\n";
    }

    // manual benchmark
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        long long acc = 0;
        for (std::size_t i = 0; i < iterations; ++i) acc += manual_sum(begin, end);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dt = t1 - t0;
        std::cout << "manual_sum:     " << dt.count() << " s, result: " << acc << "\n";
    }

    return 0;
}

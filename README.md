# POET

[![CI Status](https://img.shields.io/badge/CI-GitHub%20Actions-lightgrey)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2B-lightblue)](#)
[![Coverage](https://codecov.io/gh/DiamonDinoia/poet/branch/main/graph/badge.svg)](https://codecov.io/gh/DiamonDinoia/poet)
[![Docs Status](https://readthedocs.org/projects/poet/badge/?version=latest)](https://poet.readthedocs.io/en/latest/)

POET is a small, header-only C++ utility library that provides compile-time oriented loop and dispatch primitives to make high-performance metaprogramming simpler and safer. It focuses on three complementary capabilities:

- static_for — compile-time unrolled loops for iterating integer ranges or template packs.
- dynamic_for — efficient runtime loops implemented by emitting compile-time unrolled blocks.
- dispatch / dispatch_tuples / DispatchSet — map runtime integers or tuples to compile-time non-type template parameters for zero-cost specialization.

Why it matters
--------------
These utilities let you express algorithms that benefit from compile-time specialization (inlining, unrolling, and better optimization) while still driving them from runtime values. The result is code that is both efficient (no virtual calls, fewer branches) and expressive, useful in hotspots such as small-linear algebra, kernel selection, and template-based code generation.

In particular, POET's runtime-to-compile-time dispatch lets you select optimized, specialized code paths based on runtime choices with zero-cost abstraction.

Performance benefits
---------------------
Making iteration counts and dispatch targets visible at compile time often improves register utilization and codegen quality: the compiler can allocate registers across unrolled iterations and specialized paths, reduce spills, enable better instruction scheduling, and expose opportunities for vectorization.

Common hotspots that benefit
- Small, fixed-size dense linear algebra (small GEMM, batched matrix ops)
- Tight convolution / stencil kernels and small-kernel DSP code
- Fixed-size FFT/DFT implementations
- Hot parsing/serialization loops and state machines with fixed-state graphs
- Kernel-selection/specialization in template-based libraries where runtime choices select optimized codepaths

Minimal quick start
-------------------

Prerequisites
- A C++17-capable compiler (GCC, Clang, MSVC).
- CMake 3.20+ for CMake integration (optional).

Install / Integrate
1. Clone
```bash
git clone https://github.com/DiamonDinoia/poet.git
```

2. CMake — add_subdirectory (recommended for local development)
```cmake
add_subdirectory(path/to/poet)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE poet::poet)
```

3. CMake — FetchContent
```cmake
include(FetchContent)
FetchContent_Declare(poet GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git GIT_TAG main)
FetchContent_MakeAvailable(poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

4. Header-only usage (no build step)
- Add the repository `include/` directory to your compiler include path:
  -g++ -std=c++17 -I/path/to/poet/include ...

Basic usage examples
--------------------

Include the umbrella header:
```cpp
#include <poet/poet.hpp>
```

static_for — compile-time unrolling
```cpp
#include <iostream>

int main() {
  poet::static_for<0, 5>([](auto I) {
    // I is std::integral_constant<std::intmax_t, N>
    std::cout << decltype(I)::value << '\n';
  });
}
```

dynamic_for — runtime loop with compile-time blocks
```cpp
#include <iostream>

int main() {
  std::size_t n = 37;
  poet::dynamic_for<8>(0u, n, [](std::size_t i) {
    // body executed for i = 0..n-1 in unrolled blocks of 8
    std::cout << i << '\n';
  });
}
```

dispatch — map runtime values to compile-time templates
```cpp
#include <iostream>

struct Impl {
  template<int N>
  void operator()(int x) const {
    std::cout << "specialized N=" << N << " x=" << x << '\n';
  }
};

int main() {
  int runtime_choice = 2;
  auto params = std::make_tuple(poet::DispatchParam<poet::make_range<0, 4>>{ runtime_choice });
  poet::dispatch(Impl{}, params, 42); // calls Impl::operator()<2>(42) if in range
}
```

dispatch with DispatchSet — sparse tuple dispatch
```cpp
struct Impl2 {
  template<int A, int B>
  void operator()(const std::string &s) const {
    std::cout << A << "," << B << " -> " << s << '\n';
  }
};

int main() {
  poet::DispatchSet<int, poet::T<1,2>, poet::T<3,4>> set(1, 2);
  poet::dispatch(Impl2{}, set, std::string("hello")); // calls Impl2::operator()<1,2>
}
```

throwing dispatch — example
---------------------------
Some overloads of `dispatch` accept the tag `poet::throw_t` (alias of `throw_on_no_match_t`) as the first argument and will throw `std::runtime_error` when no compile-time match exists. This is useful when a missing specialization is a fatal configuration error.

```cpp
#include <iostream>
#include <stdexcept>

struct Impl3 {
  template<int N>
  int operator()(int x) const { return N + x; }
};

int main() {
  try {
    int runtime_choice = 10;
    auto params = std::make_tuple(poet::DispatchParam<poet::make_range<0,4>>{ runtime_choice });
    // Throws std::runtime_error because 10 is not in [0..4]
    int result = poet::dispatch(poet::throw_t, Impl3{}, params, 5);
    std::cout << result << '\n';
  } catch (const std::runtime_error &e) {
    std::cerr << "dispatch failed: " << e.what() << '\n';
  }
}
```

Documentation and License
-------------------------
- Full API docs and guides: docs/ (Sphinx/RST in repository)
- License: see LICENSE

Contributing
------------
PRs and issues welcome. Keep changes small and test performance-sensitive code.


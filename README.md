# POET

[![CI](https://github.com/DiamonDinoia/poet/actions/workflows/ci.yml/badge.svg)](https://github.com/DiamonDinoia/poet/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2B-lightblue)](https://en.cppreference.com/w/cpp/17)
[![Coverage](https://codecov.io/gh/DiamonDinoia/poet/branch/main/graph/badge.svg)](https://codecov.io/gh/DiamonDinoia/poet)
[![Docs Status](https://readthedocs.org/projects/poet/badge/?version=latest)](https://poet.readthedocs.io/en/latest/)
[![CodSpeed](https://img.shields.io/endpoint?url=https://codspeed.io/badge.json&repository=DiamonDinoia/poet)](https://codspeed.io/DiamonDinoia/poet)
[![Try on Compiler Explorer](https://img.shields.io/badge/Compiler%20Explorer-try%20it-67c52a?logo=compilerexplorer&logoColor=white)](https://diamondinoia.github.io/poet/static_for.html)

POET is a header-only C++ library for three related jobs:

- `static_for`: compile-time unrolled loops
- `dynamic_for`: runtime loops emitted as compile-time unrolled blocks
- `dispatch` / `dispatch_set`: runtime-to-compile-time specialization

It also exposes CPU detection helpers (`poet::available_registers()`, `poet::cache_line()`)
for ISA, vector-width, and cache-line queries.

## Why POET

- Exposes loop structure and dispatch choices to the compiler without forcing you into handwritten template plumbing.
- Keeps hot-path code specialized and inlinable while still accepting runtime choices.
- Helps with the two common cases where compile-time information matters most:
  - unrolled fixed-shape kernels
  - runtime selection of a small set of optimized specializations
- Stays lightweight: header-only, C++17+, and consumable from a normal CMake project.

## Include

```cpp
#include <poet/poet.hpp>
```

## Examples

Each snippet below has a runnable counterpart under [`examples/`](examples/).
Build with `-DPOET_BUILD_EXAMPLES=ON` and `ctest -R '^example_'` to run them.

### `static_for`

```cpp
poet::static_for<0, 4>([&](auto I) {
    a[I] = I * I;          // I is std::integral_constant; converts implicitly
});

// Optional block size: break a large loop into smaller isolated blocks.
poet::static_for<0, 64, 1, 8>([&](auto I) {
    total += I;
});
```

> Full source: [`examples/static_for.cpp`](examples/static_for.cpp) ·
> [![Try on Compiler Explorer][ce-badge]][ce-static-for]

### `dynamic_for`

```cpp
poet::dynamic_for<4>(0u, n, [](std::size_t i) {
    out[i] = f(i);
});

// Lane-aware form for multi-accumulator kernels.
std::array<double, 4> acc{};
poet::dynamic_for<4>(0u, n, [&](auto lane, std::size_t i) {
    acc[lane] += work(i);
});

// Compile-time step.
poet::dynamic_for<4, 2>(0, 16, [](int i) {
    use(i); // 0, 2, 4, ..., 14
});
```

C++20 adaptor (eager; treats a range as `[start, start + count)`):

```cpp
auto r = std::views::iota(0) | std::views::take(10);
r | poet::make_dynamic_for<4>([](int i) { use(i); });

std::tuple{0, 24, 2} | poet::make_dynamic_for<4>([](int i) { use(i); });
```

> Full source: [`examples/dynamic_for.cpp`](examples/dynamic_for.cpp) ·
> [![Try on Compiler Explorer][ce-badge]][ce-dynamic-for]

### `dispatch`

```cpp
struct Impl {
    template<int N>
    int operator()(int x) const { return N + x; }
};

int y = poet::dispatch(
    Impl{},
    poet::dispatch_param<poet::inclusive_range<0, 4>>{choice},
    10);

// Tuple form for cartesian products of multiple dispatch_params.
auto params = std::make_tuple(
    poet::dispatch_param<poet::inclusive_range<1, 4>>{rows},
    poet::dispatch_param<poet::inclusive_range<1, 4>>{cols});

poet::dispatch(Kernel{}, params, data);
```

> Full source: [`examples/dispatch.cpp`](examples/dispatch.cpp) ·
> [![Try on Compiler Explorer][ce-badge]][ce-dispatch]

For sparse allowed combinations, use `dispatch_set`. Pair with
`poet::throw_on_no_match` when a miss should fail:

```cpp
using Shapes = poet::dispatch_set<int,
    poet::tuple_<2, 2>, poet::tuple_<4, 4>, poet::tuple_<2, 4>>;
poet::dispatch(MatMul{}, Shapes{rows, cols}, a, b, c);

poet::dispatch(
    poet::throw_on_no_match,
    MatMul{}, Shapes{rows, cols}, a, b, c);
```

> Full source: [`examples/dispatch_set.cpp`](examples/dispatch_set.cpp) ·
> [![Try on Compiler Explorer][ce-badge]][ce-dispatch-set]

### CPU detection

```cpp
constexpr auto regs = poet::available_registers();
constexpr auto line = poet::cache_line();
```

> Full source: [`examples/cpu_info.cpp`](examples/cpu_info.cpp) ·
> [![Try on Compiler Explorer][ce-badge]][ce-cpu-info]

### Worked examples

- **Polynomial evaluation** ([`examples/polynomial.cpp`](examples/polynomial.cpp))
  picks a compile-time degree N ∈ [1, 8] from a runtime integer with
  `dispatch`, then unrolls Horner's recurrence with `static_for` so every
  coefficient lookup becomes a constant offset.
  [![Try on Compiler Explorer][ce-badge]][ce-polynomial]
- **Lane-aware dot product** ([`examples/dot_product.cpp`](examples/dot_product.cpp))
  uses `dynamic_for<L>` with a lane-aware lambda to break the serial FMA
  dependency chain into L independent accumulators — throughput goes from
  FMA-latency-bound to FMA-throughput-bound on Zen3+/SKX/Ice Lake.
  [![Try on Compiler Explorer][ce-badge]][ce-dot-product]

### Microbench (runs on Compiler Explorer)

A Google Benchmark microbench ([`examples/benchmark.cpp`](examples/benchmark.cpp))
runs the scalar dot product against `dynamic_for<4>` and `dynamic_for<8>` in
CE's *Execute* mode — the output pane shows actual ns/op timings.
[![Run benchmark on Compiler Explorer][ce-run-badge]][ce-benchmark]

```bash
cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON -DPOET_BUILD_EXAMPLE_BENCHMARK=ON
cmake --build build --target example_benchmark
./build/examples/example_benchmark
```

### Try on Compiler Explorer

| Example | Link |
| --- | --- |
| `static_for` | [![Try on Compiler Explorer][ce-badge]][ce-static-for] |
| `dynamic_for` | [![Try on Compiler Explorer][ce-badge]][ce-dynamic-for] |
| `dispatch` | [![Try on Compiler Explorer][ce-badge]][ce-dispatch] |
| `dispatch_set` | [![Try on Compiler Explorer][ce-badge]][ce-dispatch-set] |
| `cpu_info` | [![Try on Compiler Explorer][ce-badge]][ce-cpu-info] |
| `polynomial` | [![Try on Compiler Explorer][ce-badge]][ce-polynomial] |
| `dot_product` | [![Try on Compiler Explorer][ce-badge]][ce-dot-product] |
| `benchmark` (executes) | [![Run benchmark on Compiler Explorer][ce-run-badge]][ce-benchmark] |

[ce-badge]: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
[ce-run-badge]: https://img.shields.io/badge/Compiler%20Explorer-run%20benchmark-d9534f?logo=compilerexplorer&logoColor=white
[ce-static-for]: https://diamondinoia.github.io/poet/static_for.html
[ce-dynamic-for]: https://diamondinoia.github.io/poet/dynamic_for.html
[ce-dispatch]: https://diamondinoia.github.io/poet/dispatch.html
[ce-dispatch-set]: https://diamondinoia.github.io/poet/dispatch_set.html
[ce-cpu-info]: https://diamondinoia.github.io/poet/cpu_info.html
[ce-polynomial]: https://diamondinoia.github.io/poet/polynomial.html
[ce-dot-product]: https://diamondinoia.github.io/poet/dot_product.html
[ce-benchmark]: https://diamondinoia.github.io/poet/benchmark.html

## Install

POET is header-only. The simplest local integration is:

```cmake
add_subdirectory(path/to/poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

Installed-package usage:

```cmake
find_package(poet CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE poet::poet)
```

FetchContent usage:

```cmake
include(FetchContent)
FetchContent_Declare(
  poet
  GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git
  GIT_TAG main
)
FetchContent_MakeAvailable(poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

For non-CMake builds, add `include/` to your compiler include path.

## Benchmarks

POET ships with benchmarks for `static_for`, `dynamic_for`, and `dispatch`. The main takeaways are:

- `dynamic_for` helps when lane-aware callbacks break dependency chains.
- `static_for` benefits from tuned block sizes on heavier kernels.
- `dispatch` helps when a runtime choice unlocks compile-time specialization.

Charts are generated by CI across GCC and Clang via the [benchmark workflow](https://github.com/DiamonDinoia/poet/actions/workflows/benchmarks.yml):

**dynamic_for**: Multi-accumulator ILP. Compile-time lane indices let the compiler
maintain independent accumulator chains and break serial dependency
bottlenecks.

![dynamic_for speedup](https://raw.githubusercontent.com/DiamonDinoia/poet/benchmark-results/benchmark-results/dynamic_for_speedup.svg)

**static_for**: Register-aware block sizing. Matching the unroll factor to
available SIMD registers avoids spill-driven slowdowns while preserving
throughput.

![static_for speedup](https://raw.githubusercontent.com/DiamonDinoia/poet/benchmark-results/benchmark-results/static_for_speedup.svg)

**dispatch**: Compile-time specialization. When `N` is known at compile time,
the compiler can fully unroll evaluation loops, constant-fold coefficients,
and schedule instructions more aggressively.

![dispatch optimization](https://raw.githubusercontent.com/DiamonDinoia/poet/benchmark-results/benchmark-results/dispatch_optimization.svg)

**Cross-compiler consistency**: The same optimization patterns produce speedups
across both GCC and Clang rather than depending on a single compiler.

![cross-compiler overview](https://raw.githubusercontent.com/DiamonDinoia/poet/benchmark-results/benchmark-results/cross_compiler_overview.svg)

See [docs/guides/benchmarks.rst](docs/guides/benchmarks.rst) and the
[CodSpeed dashboard](https://codspeed.io/DiamonDinoia/poet) for run details
and regression tracking.

## Docs

- Guides and API docs: [poet.readthedocs.io](https://poet.readthedocs.io/en/latest/)
- Install guide: [docs/install.rst](docs/install.rst)
- API entry point: [include/poet/poet.hpp](include/poet/poet.hpp)

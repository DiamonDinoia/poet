# POET

[![CI](https://github.com/flatironinstitute/poet/actions/workflows/ci.yml/badge.svg)](https://github.com/flatironinstitute/poet/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2B-lightblue)](https://en.cppreference.com/w/cpp/17)
[![Coverage](https://codecov.io/gh/flatironinstitute/poet/branch/main/graph/badge.svg)](https://codecov.io/gh/flatironinstitute/poet)
[![Docs](https://github.com/flatironinstitute/poet/actions/workflows/docs.yml/badge.svg)](https://flatironinstitute.github.io/poet/docs/)
[![Try on Compiler Explorer](https://img.shields.io/badge/Compiler%20Explorer-try%20it-67c52a?logo=compilerexplorer&logoColor=white)](https://flatironinstitute.github.io/poet/static_for.html)

POET is a header-only C++ library for three related jobs:

- `static_for`: compile-time unrolled loops
- `dynamic_for`: runtime loops emitted as compile-time unrolled blocks, exactly
  `Unroll` bodies per back-edge
- `dispatch` / `dispatch_set`: runtime-to-compile-time specialization

It also exposes CPU detection helpers (`poet::available_registers()`, `poet::cache_line()`)
for ISA, vector-width, and cache-line queries.

Requires C++17 or later. Hot paths stay specialized and inlinable without
handwritten template plumbing.

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

// Optional block size: break a large loop into smaller outlined blocks.
poet::static_for<0, 64, 1, 8>([&](auto I) {
    total += I;
});
```

[![Source: static_for.cpp][src-badge-static-for]](examples/static_for.cpp) [![Try on Compiler Explorer][ce-badge]][ce-static-for]

### `dynamic_for`

`Unroll` is exact: the emitted main loop carries `Unroll` copies of the body per
back-edge and the compiler does not unroll it again, `Unroll` of 1 and a
`-funroll-loops` build included.

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

C++20 adaptor (eager; iterates a random-access range's elements):

```cpp
auto r = std::views::iota(0) | std::views::take(10);
r | poet::make_dynamic_for<4>([](int i) { use(i); });

std::tuple{0, 24, 2} | poet::make_dynamic_for<4>([](int i) { use(i); });
```

[![Source: dynamic_for.cpp][src-badge-dynamic-for]](examples/dynamic_for.cpp) [![Try on Compiler Explorer][ce-badge]][ce-dynamic-for]

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

[![Source: dispatch.cpp][src-badge-dispatch]](examples/dispatch.cpp) [![Try on Compiler Explorer][ce-badge]][ce-dispatch]

A miss is silent: the non-throwing overloads return a default-constructed result
without calling the functor. Prefix with `poet::throw_on_no_match` to get a
`poet::no_match_error` instead.

For sparse allowed combinations, use `dispatch_set`. Unlike `dispatch_param`,
this path only calls the template form `f.template operator()<Vs...>(args...)`:

```cpp
struct MatMul {
    template<int Rows, int Cols>
    void operator()(const float *a, const float *b, float *c) const;
};

using Shapes = poet::dispatch_set<int,
    poet::tuple_<2, 2>, poet::tuple_<4, 4>, poet::tuple_<2, 4>>;

poet::dispatch(MatMul{}, Shapes{rows, cols}, a, b, c);

poet::dispatch(
    poet::throw_on_no_match,
    MatMul{}, Shapes{rows, cols}, a, b, c);
```

[![Source: dispatch_set.cpp][src-badge-dispatch-set]](examples/dispatch_set.cpp) [![Try on Compiler Explorer][ce-badge]][ce-dispatch-set]

### CPU detection

```cpp
constexpr auto regs = poet::available_registers();
constexpr auto line = poet::cache_line();
```

[![Source: cpu_info.cpp][src-badge-cpu-info]](examples/cpu_info.cpp) [![Try on Compiler Explorer][ce-badge]][ce-cpu-info]

### Worked examples

- **Polynomial evaluation** ([`examples/polynomial.cpp`](examples/polynomial.cpp))
  picks a compile-time degree N ∈ [1, 8] from a runtime integer with
  `dispatch`, then unrolls Horner's recurrence with `static_for` so every
  coefficient lookup becomes a constant offset.
  [![Try on Compiler Explorer][ce-badge]][ce-polynomial]
- **Lane-aware dot product** ([`examples/dot_product.cpp`](examples/dot_product.cpp))
  uses `dynamic_for<L>` with a lane-aware lambda to break the serial FMA
  dependency chain into L independent accumulators. Throughput goes from
  FMA-latency-bound to FMA-throughput-bound.
  [![Try on Compiler Explorer][ce-badge]][ce-dot-product]

### Microbench (runs on Compiler Explorer)

A Google Benchmark microbench ([`examples/benchmark.cpp`](examples/benchmark.cpp))
runs the scalar dot product against `dynamic_for<4>` and `dynamic_for<8>` in
CE's *Execute* mode. The output pane shows actual ns/op timings.
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
[ce-static-for]: https://flatironinstitute.github.io/poet/static_for.html
[ce-dynamic-for]: https://flatironinstitute.github.io/poet/dynamic_for.html
[ce-dispatch]: https://flatironinstitute.github.io/poet/dispatch.html
[ce-dispatch-set]: https://flatironinstitute.github.io/poet/dispatch_set.html
[ce-cpu-info]: https://flatironinstitute.github.io/poet/cpu_info.html
[ce-polynomial]: https://flatironinstitute.github.io/poet/polynomial.html
[ce-dot-product]: https://flatironinstitute.github.io/poet/dot_product.html
[ce-benchmark]: https://flatironinstitute.github.io/poet/benchmark.html
[src-badge-static-for]: https://img.shields.io/badge/source-static__for.cpp-1f6feb?logo=cplusplus&logoColor=white
[src-badge-dynamic-for]: https://img.shields.io/badge/source-dynamic__for.cpp-1f6feb?logo=cplusplus&logoColor=white
[src-badge-dispatch]: https://img.shields.io/badge/source-dispatch.cpp-1f6feb?logo=cplusplus&logoColor=white
[src-badge-dispatch-set]: https://img.shields.io/badge/source-dispatch__set.cpp-1f6feb?logo=cplusplus&logoColor=white
[src-badge-cpu-info]: https://img.shields.io/badge/source-cpu__info.cpp-1f6feb?logo=cplusplus&logoColor=white

## Install

POET is header-only. In a local CMake project:

```cmake
add_subdirectory(path/to/poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

As an installed package:

```cmake
find_package(poet CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE poet::poet)
```

Via FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
  poet
  GIT_REPOSITORY https://github.com/flatironinstitute/poet.git
  GIT_TAG main
)
FetchContent_MakeAvailable(poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

For non-CMake builds, generate the version header once (`include/poet/version.hpp`
is not tracked in git), then add `include/` to the compiler include path:

```bash
cmake -P cmake/GenerateVersion.cmake
```

## Benchmarks

POET ships with benchmarks for `static_for`, `dynamic_for`, and `dispatch`.

Multi-accumulator ILP with `dynamic_for`. Compile-time lane indices let the
compiler maintain independent accumulator chains and break serial dependency
bottlenecks.

![dynamic_for speedup](https://raw.githubusercontent.com/flatironinstitute/poet/benchmark-results/benchmark-results/dynamic_for_speedup.svg)

Register-aware block sizing with `static_for`. Matching the unroll factor to
available SIMD registers avoids spill-driven slowdowns while preserving
throughput.

![static_for speedup](https://raw.githubusercontent.com/flatironinstitute/poet/benchmark-results/benchmark-results/static_for_speedup.svg)

Compile-time specialization with `dispatch`. When `N` is known at compile time,
the compiler fully unrolls evaluation loops, constant-folds coefficients, and
schedules instructions more aggressively.

![dispatch optimization](https://raw.githubusercontent.com/flatironinstitute/poet/benchmark-results/benchmark-results/dispatch_optimization.svg)

The same patterns produce speedups on both GCC and Clang.

![cross-compiler overview](https://raw.githubusercontent.com/flatironinstitute/poet/benchmark-results/benchmark-results/cross_compiler_overview.svg)

See [docs/guides/benchmarks.rst](docs/guides/benchmarks.rst) for run details.

## Docs

- Guides and API docs: [flatironinstitute.github.io/poet/docs](https://flatironinstitute.github.io/poet/docs/)
- Install guide: [docs/install.rst](docs/install.rst)
- API entry point: [include/poet/poet.hpp](include/poet/poet.hpp)

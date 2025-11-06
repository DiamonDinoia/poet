# POET

[![CI Status](https://img.shields.io/badge/CI-GitHub%20Actions-lightgrey)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2B-lightblue)](#)

## Introduction

POET (Performance Optimized Excessive Templates) is a header-only collection of modern C++ building blocks wrapped in a thin CMake package. The library ships with batteries-included tooling so teams can adopt strict warnings, sanitizers, and static analysis defaults without having to maintain a bespoke infrastructure for each project.

## Features

- Header-only interface target `poet::poet`.
- Curated warning levels for GCC, Clang/AppleClang, and MSVC via `PoetWarnings.cmake`.
- Opt-in sanitizer configuration (Address, Undefined, Thread, Memory) with platform-aware safeguards.
- Optional static analysis hooks for clang-tidy and cppcheck.
- CPack integration for generating relocatable archives.

### Compile-time iteration utilities

POET ships `poet::static_for`, a header-only facility for unrolled
compile-time loops. Functors can expose `template <auto I>` call operators and
are invoked for each value in the requested range, including support for
negative steps:

```cpp
#include <poet/core/static_for.hpp>

struct printer {
  template <auto I>
  constexpr void operator()() const {
    // Replace with a real compile-time action.
    static_assert(I >= 0);
  }
};

constexpr void enumerate() {
  poet::static_for<0, 4>(printer{});         // Iterates 0, 1, 2, 3
  poet::static_for<3, -1, -1>(printer{});    // Iterates 3, 2, 1, 0
}
```

For runtime-controlled loops, `poet::dynamic_for` bridges the same
unrolling machinery with a runtime entry point. The helper accepts an inclusive
`begin` and exclusive `end` bound and invokes a callable with the runtime index
for each step:

```cpp
#include <poet/core/dynamic_for.hpp>
#include <vector>

std::vector<std::size_t> visit(std::size_t begin, std::size_t end) {
  std::vector<std::size_t> visited;
  poet::dynamic_for<4>(begin, end, [&](std::size_t index) {
    visited.push_back(index);
  });
  return visited;
}
```

Small unroll factors (the default is `8`) tend to deliver the best balance
between instruction-cache residency and loop efficiency. The helper enforces an
inclusive upper bound of `poet::kMaxStaticLoopBlock` (currently `256`)
iterations per block, so the maximum supported unroll factor is `256`, matching
the compile-time unroller's limit.

### Runtime to compile-time dispatch

When template parameters must be selected from runtime values, use
`poet::dispatch` to probe a Cartesian product of integer sequences and
invoke a templated functor once a match is found:

```cpp
#include <poet/core/static_dispatch.hpp>

struct kernel {
  template <int Width, int Height>
  void operator()(int scale) const;
};

auto params = std::make_tuple(
    poet::DispatchParam<poet::make_range<0, 7>>{requested_width},
    poet::DispatchParam<poet::make_range<0, 7>>{requested_height});

poet::dispatch(kernel{}, params, current_scale);
```

If no combination matches the runtime inputs, the functor is never invoked.

### Configuration Options

| Option | Description | Default |
| --- | --- | --- |
| `POET_WARNINGS_AS_ERRORS` | Promote compiler warnings to errors. | `ON` |
| `POET_STRICT_WARNINGS` | Apply the curated warning profile to `poet::poet`. | `ON` |
| `POET_ENABLE_ASAN` / `POET_ENABLE_UBSAN` / `POET_ENABLE_TSAN` / `POET_ENABLE_MSAN` | Enable the respective sanitizer when supported by the active compiler. | `OFF` |
| `POET_ENABLE_CLANG_TIDY` | Configure `clang-tidy` with warnings-as-errors for the `poet::poet` target. | `OFF` |
| `POET_CLANG_TIDY_CHECKS` | Optional override for the checks executed by `clang-tidy`. | `""` |
| `POET_ENABLE_CPPCHECK` | Run `cppcheck` during the build. | `OFF` |
| `POET_CPPCHECK_OPTIONS` | Extra arguments forwarded to `cppcheck`. | `--enable=warning,style,performance,portability` |

## Getting Started

### Using `find_package`

After installing POET (for example via `cmake --install` or a package manager), locate the package from your project:

```cmake
find_package(poet CONFIG REQUIRED)

add_executable(your_target src/main.cpp)
target_link_libraries(your_target PRIVATE poet::poet)
```

The imported target propagates include paths, warnings, sanitizer preferences, and static-analysis hooks.

### FetchContent / CPM.cmake

You can also consume POET directly from source without a prior install:

```cmake
FetchContent_Declare(
  poet
  GIT_REPOSITORY https://github.com/<org>/poet.git
  GIT_TAG <commit-or-tag>
)
FetchContent_MakeAvailable(poet)
```

```cmake
CPMAddPackage(
  NAME poet
  GIT_REPOSITORY https://github.com/<org>/poet.git
  GIT_TAG <commit-or-tag>
)
```

### Manual include usage

When vendoring POET without CMake, add the `include/` directory to your compiler’s search path and include the headers you need. Link your targets against the headers normally—no additional libraries are required because POET is header-only. Note that the automatic warning, sanitizer, and tooling integrations are only available through the CMake modules.

## Supported Compilers & Platforms

POET targets C++17 and is routinely built with:

- GCC 11 or newer on Linux.
- Clang 13 or newer on Linux and macOS (AppleClang).
- MSVC 19.30 or newer on Windows.

Earlier toolchains may work but are not part of the official support matrix. Platform-specific sanitizer limitations (for example, MemorySanitizer requiring Linux) are enforced by the helper modules.

## Building the Project

```bash
cmake -S . -B build
cmake --build build
```

Use `cmake --install build` to install headers and CMake package metadata into your chosen prefix. The generated archive from CPack can be produced with `cpack` inside the build directory.

To execute the automated checks after configuring, either run `ctest --output-on-failure` from the build directory or provide the build tree explicitly:

```bash
ctest --output-on-failure --test-dir build
```

## Building Tests

The `tests/` directory hosts automated checks. Once populated, enable its targets in your preferred way (for example by adding `add_subdirectory(tests)` to a downstream project) and rebuild:

```bash
cmake --build build --target <test-target>
ctest --output-on-failure --test-dir build
```

The Catch2 label `[static_for]` exercises the compile-time iteration utilities, with the `[static_for][loop]` and `[static_for][dispatch]` sections covering `static_loop` and `static_for` behaviour respectively.

Contributions that expand this directory are welcome—see the Contributing section below for details.

## Sanitizer Usage

To enable sanitizers, toggle the corresponding cache variables at configure time:

```bash
cmake -S . -B build \
  -DPOET_ENABLE_ASAN=ON \
  -DPOET_ENABLE_UBSAN=ON
```

POET automatically detects whether the active compiler supports each sanitizer and applies the appropriate compile and link options. MemorySanitizer is limited to Linux hosts, and MSVC currently supports AddressSanitizer only. Combine sanitizers judiciously—some toolchains do not allow multiple sanitizers simultaneously.

## Contributing

Issues and pull requests are welcome. Please:

1. Configure with the default warnings and tooling options enabled.
2. Run the sanitizer and static analysis options relevant to your change.
3. Add or update examples/tests where practical to demonstrate new behavior.
4. Keep pull requests focused and documented, referencing any related issues.

## Changelog & Versioning

Track notable changes in `CHANGELOG.md` (to be added alongside the first tagged version) and in the GitHub Releases page. Each release will highlight breaking changes, new features, and fixes.

## License

POET is distributed under the terms of the [MIT License](LICENSE).

# POET examples

Self-contained, runnable examples for each public POET primitive. Each file is a
single translation unit you can copy, build, and link from the docs.

## Build & run

```bash
cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -R '^example_' --output-on-failure
```

The Google Benchmark example is opt-in to keep the default example build
dependency-free. Enable it with `-DPOET_BUILD_EXAMPLE_BENCHMARK=ON` (the
benchmarks/ directory's existing Google Benchmark target is reused when
`-DPOET_BUILD_BENCHMARKS=ON` is also set).

## Index

| File | Topic | Guide | Compiler Explorer |
| --- | --- | --- | --- |
| [`static_for.cpp`](static_for.cpp) | compile-time unrolled loops | [static_for.rst](../docs/guides/static_for.rst) | [![CE][ce-badge]](https://diamondinoia.github.io/poet/static_for.html) |
| [`dynamic_for.cpp`](dynamic_for.cpp) | runtime loops, lane-aware blocks | [dynamic_for.rst](../docs/guides/dynamic_for.rst) | [![CE][ce-badge]](https://diamondinoia.github.io/poet/dynamic_for.html) |
| [`dispatch.cpp`](dispatch.cpp) | runtime → compile-time specialization | [dispatch.rst](../docs/guides/dispatch.rst) | [![CE][ce-badge]](https://diamondinoia.github.io/poet/dispatch.html) |
| [`dispatch_set.cpp`](dispatch_set.cpp) | sparse dispatch + `throw_on_no_match` | [dispatch.rst](../docs/guides/dispatch.rst) | [![CE][ce-badge]](https://diamondinoia.github.io/poet/dispatch_set.html) |
| [`cpu_info.cpp`](cpu_info.cpp) | `available_registers` / `cache_line` | — | [![CE][ce-badge]](https://diamondinoia.github.io/poet/cpu_info.html) |
| [`polynomial.cpp`](polynomial.cpp) | Horner specialised on degree (`dispatch` + `static_for`) | — | [![CE][ce-badge]](https://diamondinoia.github.io/poet/polynomial.html) |
| [`dot_product.cpp`](dot_product.cpp) | lane-aware multi-accumulator dot product | — | [![CE][ce-badge]](https://diamondinoia.github.io/poet/dot_product.html) |
| [`benchmark.cpp`](benchmark.cpp) | Google Benchmark microbench, runs on CE | — | [![CE-run][ce-run-badge]](https://diamondinoia.github.io/poet/benchmark.html) |

[ce-badge]: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
[ce-run-badge]: https://img.shields.io/badge/Compiler%20Explorer-run%20benchmark-d9534f?logo=compilerexplorer&logoColor=white

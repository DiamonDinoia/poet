Benchmark Results
=================

POET includes a comprehensive benchmark suite built on `nanobench <https://github.com/martinus/nanobench>`_ that measures real-world performance of its core primitives.  CI runs these benchmarks across multiple GCC and Clang versions and generates SVG charts automatically.

How to run locally
------------------

.. code-block:: bash

   # Build benchmarks
   cmake -B build -DPOET_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build --target poet_benchmarks

   # Run all benchmarks
   cmake --build build --target poet_run_bench

   # Multi-compiler sweep (requires gcc/clang installed)
   bash scripts/bench_all.sh

   # Generate charts (requires matplotlib)
   pip install matplotlib
   python3 scripts/generate_charts.py --results-root results --output-dir docs/benchmarks

dynamic_for: multi-accumulator ILP
-----------------------------------

.. image:: ../benchmarks/dynamic_for_speedup.svg
   :alt: dynamic_for speedup chart
   :width: 800px

**Why it's faster:**  A scalar reduction ``for (i=0; i<N; ++i) acc += f(i)`` creates a serial dependency chain — each addition waits for the previous one, leaving most execution ports idle.  ``dynamic_for`` with lane callbacks splits the accumulation into independent chains:

.. code-block:: cpp

   std::array<double, Unroll> accs{};
   poet::dynamic_for<Unroll>(0u, N, [&](auto lane, std::size_t i) {
       accs[lane] += heavy_work(i, salt);
   });

Because ``lane`` is a compile-time constant (``std::integral_constant``), the compiler can allocate each accumulator to a separate register, breaking the dependency bottleneck and enabling instruction-level parallelism.  The register-aware tuning (``lanes_64 * 2`` accumulators) matches the available SIMD register file to avoid spill pressure.

static_for: register-aware block sizing
----------------------------------------

.. image:: ../benchmarks/static_for_speedup.svg
   :alt: static_for speedup chart
   :width: 800px

**Why it's faster:**  ``static_for`` unrolls a compile-time loop into individual template instantiations.  The key insight is that the *block size* (unroll factor) must match the hardware register file:

- **Too small** (e.g. 1): no ILP, serial execution.
- **Optimal** (e.g. ``lanes_64 * 2`` for multi-acc, ``vec_regs * lanes_64 / 2`` for map): peak throughput, minimal spills.
- **Too large** (e.g. ``4 * optimal``): register spill pressure degrades performance — objdump shows thousands of stack references in the hot loop.

The benchmark validates the heuristic ``optimal_accs = lanes_64 * 2`` by showing it consistently hits peak throughput across compilers.

dispatch: compile-time specialization
--------------------------------------

.. image:: ../benchmarks/dispatch_optimization.svg
   :alt: dispatch optimization chart
   :width: 800px

**Why it's faster:**  ``poet::dispatch`` maps a runtime integer to a compile-time template parameter.  The benchmark uses Horner polynomial evaluation to demonstrate the impact:

- **Runtime N**: The compiler emits a counted loop with a data dependency per iteration.  It cannot unroll because the trip count is in a register.
- **Dispatched N**: Each specialization sees N as a compile-time constant.  The compiler fully unrolls the evaluation, constant-folds operations, and schedules instructions optimally.

.. code-block:: cpp

   // Runtime: compiler sees a loop
   double horner_runtime(const double *c, int n, double x) {
       double r = c[n - 1];
       for (int i = n - 2; i >= 0; --i)
           r = r * x + c[i];
       return r;
   }

   // Dispatched: compiler unrolls completely
   template <int N>
   double horner_compiletime(const double *c, double x) {
       double r = c[N - 1];
       poet::static_for<0, N - 1>([&](auto I) {
           constexpr int i = N - 2 - int(decltype(I)::value);
           r = r * x + c[i];
       });
       return r;
   }

   // Map runtime n → compile-time N
   poet::dispatch(HornerDispatch{coeffs, x},
                  poet::DispatchParam<poet::make_range<4, 32>>{n});

The speedup grows with N because larger polynomials offer more unrolling opportunity.

Cross-compiler consistency
--------------------------

.. image:: ../benchmarks/cross_compiler_overview.svg
   :alt: cross-compiler overview chart
   :width: 800px

**Why it matters:**  POET's performance patterns are not compiler-specific tricks.  The chart shows that all three optimization categories (``dynamic_for``, ``static_for``, ``dispatch``) deliver consistent speedups across both GCC and Clang.  This robustness comes from exposing information to the compiler (compile-time constants, unrolled loops, template specializations) rather than relying on compiler-specific heuristics.

Benchmark methodology
---------------------

- All benchmarks use ``ankerl::nanobench`` with ``minEpochTime(50ms)`` and ``relative(true)`` for stable, comparable measurements.
- A ``xorshift32`` PRNG seeded from ``volatile g_salt`` prevents dead code elimination without introducing PRNG quality variance between compilers.
- Benchmarks avoid FMA chains (which produce GCC/Clang asymmetry) in favor of integer accumulation or simple multiply-add patterns.
- CI runs on ``ubuntu-24.04`` without ``-march=native`` for reproducibility; local ``bench_all.sh`` includes a ``native`` variant for ISA-specific results.

Benchmarks
==========

POET includes benchmarks for ``static_for``, ``dynamic_for``, and ``dispatch``.

Run locally
-----------

.. code-block:: bash

   cmake -S . -B build -DPOET_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build --target poet_benchmarks
   cmake --build build --target poet_run_bench

For the multi-compiler sweep:

.. code-block:: bash

   bash scripts/bench_all.sh

Key takeaways
-------------

- ``dynamic_for`` helps most when lane-aware callbacks create independent accumulator chains.
- ``static_for`` benefits from tuned block sizes on heavier loop bodies.
- ``dispatch`` helps when a runtime choice unlocks compile-time specialization.

See the repository README and CodSpeed dashboard for current charts.

The multi-compiler sweep driver lives at `scripts/bench_all.sh
<https://github.com/DiamonDinoia/poet/blob/main/scripts/bench_all.sh>`_.

Run a microbench on Compiler Explorer
-------------------------------------

A small Google Benchmark microbench lives in
`examples/benchmark.cpp
<https://github.com/DiamonDinoia/poet/blob/main/examples/benchmark.cpp>`_;
it compares scalar dot product against ``dynamic_for<4>`` and
``dynamic_for<8>``. The Compiler Explorer link below compiles, links
Google Benchmark, and runs in *Execute* mode, so the timings appear in
the output pane:

|ce-run-badge| `Run benchmark on Compiler Explorer <https://diamondinoia.github.io/poet/benchmark.html>`_

.. |ce-run-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-run%20benchmark-d9534f?logo=compilerexplorer&logoColor=white
   :alt: Run benchmark on Compiler Explorer

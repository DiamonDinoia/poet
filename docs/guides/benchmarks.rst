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

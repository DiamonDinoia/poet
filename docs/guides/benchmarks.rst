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

- ``dynamic_for`` breaks dependency chains through lane-aware callbacks.
- ``static_for`` benefits from tuned block sizes on heavier loop bodies.
- ``dispatch`` turns a runtime choice into compile-time specialization.

See the repository README for current charts.

Run a microbench on Compiler Explorer
-------------------------------------

The microbench in `examples/benchmark.cpp
<https://github.com/flatironinstitute/poet/blob/main/examples/benchmark.cpp>`_
compares scalar dot product against ``dynamic_for<4>`` and ``dynamic_for<8>``.
The link below runs it on Compiler Explorer in *Execute* mode and shows
timings in the output pane:

|ce-run-badge| `Run benchmark on Compiler Explorer <https://flatironinstitute.github.io/poet/benchmark.html>`_

.. |ce-run-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-run%20benchmark-d9534f?logo=compilerexplorer&logoColor=white
   :alt: Run benchmark on Compiler Explorer

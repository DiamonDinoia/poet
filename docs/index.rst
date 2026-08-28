POET
====

POET is a header-only C++ library for compile-time unrolling and runtime-to-compile-time specialization.

Project links
-------------

- `GitHub repository <https://github.com/flatironinstitute/poet>`_
- `Issue tracker <https://github.com/flatironinstitute/poet/issues>`_
- `Releases <https://github.com/flatironinstitute/poet/releases>`_
- `License (MIT) <https://github.com/flatironinstitute/poet/blob/main/LICENSE>`_
- `Single-header build <https://github.com/flatironinstitute/poet/tree/single-header>`_

Core primitives
---------------

- ``static_for``: compile-time unrolled loops over integer ranges
- ``dynamic_for``: runtime loops emitted as compile-time unrolled blocks
- ``dispatch`` / ``dispatch_set``: runtime choice mapped to compile-time specializations
- CPU detection: ISA, vector-width, and cache-line helpers (``poet::available_registers()``, ``poet::cache_line()``)

Quick start
-----------

.. note::

   Each example runs online on Compiler Explorer. The link pulls the
   `single-header build <https://github.com/flatironinstitute/poet/tree/single-header>`_
   from GitHub at compile time via Compiler Explorer's URL-include feature.

   - |ce-badge| `static_for <https://flatironinstitute.github.io/poet/static_for.html>`_
   - |ce-badge| `dynamic_for <https://flatironinstitute.github.io/poet/dynamic_for.html>`_
   - |ce-badge| `dispatch <https://flatironinstitute.github.io/poet/dispatch.html>`_
   - |ce-badge| `dispatch_set <https://flatironinstitute.github.io/poet/dispatch_set.html>`_
   - |ce-badge| `cpu_info <https://flatironinstitute.github.io/poet/cpu_info.html>`_
   - |ce-badge| `polynomial: Horner via dispatch + static_for <https://flatironinstitute.github.io/poet/polynomial.html>`_
   - |ce-badge| `dot_product: lane-aware ILP <https://flatironinstitute.github.io/poet/dot_product.html>`_
   - |ce-run-badge| `benchmark: Google Benchmark microbench, runs on CE <https://flatironinstitute.github.io/poet/benchmark.html>`_

   Source for each example lives in
   `examples/ <https://github.com/flatironinstitute/poet/tree/main/examples>`_.

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
.. |ce-run-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-run%20benchmark-d9534f?logo=compilerexplorer&logoColor=white
   :alt: Run benchmark on Compiler Explorer

Include the umbrella header:

.. code-block:: cpp

   #include <poet/poet.hpp>

Minimal examples:

.. code-block:: cpp

   poet::static_for<0, 4>([](auto I) {
       use_index(I);
   });

   poet::dynamic_for<4>(0u, n, [](std::size_t i) {
       out[i] = f(i);
   });

   auto y = poet::dispatch(
       Kernel{},
       poet::dispatch_param<poet::inclusive_range<0, 4>>{choice},
       x);

Notes
-----

- ``dynamic_for`` helps with the lane-aware callback form for multi-accumulator work.
- The C++20 ``make_dynamic_for`` adaptor is eager and needs a random-access range.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   install

.. toctree::
   :maxdepth: 2
   :caption: Guides

   guides/static_for
   guides/dynamic_for
   guides/dispatch
   guides/benchmarks

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/library_root

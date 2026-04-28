POET
====

POET is a header-only C++ library for compile-time unrolling and runtime-to-compile-time specialization.

Project links
-------------

- `GitHub repository <https://github.com/DiamonDinoia/poet>`_
- `Issue tracker <https://github.com/DiamonDinoia/poet/issues>`_
- `Releases <https://github.com/DiamonDinoia/poet/releases>`_
- `License (MIT) <https://github.com/DiamonDinoia/poet/blob/main/LICENSE>`_
- `CodSpeed dashboard <https://codspeed.io/DiamonDinoia/poet>`_
- `Single-header build <https://github.com/DiamonDinoia/poet/tree/single-header>`_

Core primitives
---------------

- ``static_for``: compile-time unrolled loops over integer ranges
- ``dynamic_for``: runtime loops emitted as compile-time unrolled blocks
- ``dispatch`` / ``dispatch_set``: runtime choice mapped to compile-time specializations
- CPU detection: ISA, vector-width, and cache-line helpers (``poet::available_registers()``, ``poet::cache_line()``)

Quick start
-----------

.. note::

   **Try it online.** Each example has a one-click Compiler Explorer link
   that pulls the
   `single-header build <https://github.com/DiamonDinoia/poet/tree/single-header>`_
   from GitHub at compile time via Compiler Explorer's URL-include feature.

   - |ce-badge| `static_for <https://diamondinoia.github.io/poet/static_for.html>`_
   - |ce-badge| `dynamic_for <https://diamondinoia.github.io/poet/dynamic_for.html>`_
   - |ce-badge| `dispatch <https://diamondinoia.github.io/poet/dispatch.html>`_
   - |ce-badge| `dispatch_set <https://diamondinoia.github.io/poet/dispatch_set.html>`_
   - |ce-badge| `cpu_info <https://diamondinoia.github.io/poet/cpu_info.html>`_
   - |ce-badge| `polynomial — Horner via dispatch + static_for <https://diamondinoia.github.io/poet/polynomial.html>`_
   - |ce-badge| `dot_product — lane-aware ILP <https://diamondinoia.github.io/poet/dot_product.html>`_
   - |ce-run-badge| `benchmark — Google Benchmark microbench, runs on CE <https://diamondinoia.github.io/poet/benchmark.html>`_

   Source for each example lives in
   `examples/ <https://github.com/DiamonDinoia/poet/tree/main/examples>`_;
   regenerate the links with
   `tools/make_godbolt_links.py <https://github.com/DiamonDinoia/poet/blob/main/tools/make_godbolt_links.py>`_.

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

- ``poet::inclusive_range<Start, End>`` is inclusive on both ends.
- ``dynamic_for`` is most useful with the lane-aware callback form for multi-accumulator work.
- The C++20 ``make_dynamic_for`` adaptor is eager.

Next reads
----------

- :doc:`install`
- :doc:`guides/static_for`
- :doc:`guides/dynamic_for`
- :doc:`guides/dispatch`
- :doc:`guides/benchmarks`

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

POET
====

POET is a header-only C++ library for compile-time unrolling and runtime-to-compile-time specialization.

Core primitives
---------------

- ``static_for``: compile-time unrolled loops over integer ranges
- ``dynamic_for``: runtime loops emitted as compile-time unrolled blocks
- ``dispatch`` / ``dispatch_set``: runtime choice mapped to compile-time specializations
- CPU detection: ISA, vector-width, and cache-line helpers (``poet::available_registers()``, ``poet::cache_line()``)

Quick start
-----------

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
